#include <cmath>
#include <cstdio>
#include <iostream>
#include <vector>

#include "ceres/ceres.h"
#include "ceres/rotation.h"

#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

template <typename T>
void project(const T* const camera_int,
             const T* const camera_ext,
             const T* const point,
             T* projected) {
    T p_t[3];
    p_t[0] = point[0] - camera_ext[3];
    p_t[1] = point[1] - camera_ext[4];
    p_t[2] = point[2] - camera_ext[5];
    T p[3];
    ceres::AngleAxisRotatePoint(camera_ext, p_t, p);

    // looking down the posative x axis;
    // x on the sensor comes from -y in the world
    // y on the sensor comes from z in the world
    T xp = -p[1] / p[0];
    T yp = p[2] / p[0];

    // Apply second and fourth order radial distortion.
    // const T& l1 = camera_int[1];
    // const T& l2 = camera_int[2];
    // T r2 = xp*xp + yp*yp;
    // T distortion = T(1.0) + r2  * (l1 + l2  * r2);
    T distortion = T(1.0);

    // Compute final projected point position.
    const T& focal = camera_int[0];
    projected[0] = focal * distortion * xp;
    projected[1] = focal * distortion * yp;
    projected[2] = p[0];
}

// Templated pinhole camera model for used with Ceres.  The camera is
// parameterized using 9 parameters: 3 for rotation, 3 for translation, 1 for
// focal length and 2 for radial distortion. The principal point is not modeled
// (i.e. it is assumed be located at the image center).
struct SnavelyReprojectionError {
    SnavelyReprojectionError(double observed_x, double observed_y)
        : observed_x(observed_x), observed_y(observed_y) {}

    template <typename T>
        bool operator()(
                const T* const camera_int,
                const T* const camera_ext,
                const T* const point,
                T* residuals) const {
            T projected[3];
            project(camera_int, camera_ext, point, projected);

            // The error is the difference between the predicted and observed position.
            residuals[0] = projected[0] - T(observed_x);
            residuals[1] = projected[1] - T(observed_y);

            return true;
        }

    // Factory to hide the construction of the CostFunction object from
    // the client code.
    static ceres::CostFunction* Create(
            const double observed_x,
            const double observed_y) {
        return (new ceres::AutoDiffCostFunction<SnavelyReprojectionError, 2, 1, 6, 3>(
                    new SnavelyReprojectionError(observed_x, observed_y)));
    }

    double observed_x;
    double observed_y;
};

struct TreeProblem {
    struct LED {
        double pos[3] = {
            0.0, 0.0, 0.0,
        };
        
        std::vector<Point> original_points;
        std::vector<bool> visible;
    };
    struct Camera {
        double camera_ext[7] = {
            0.0, 0.0, 0.0,
            0.0, 0.0, 0.0,
        };
    };
    
    double camera_int[1] = {
        1.0,
    };
    
    double img_size[2] = {1920, 1080};
    
    std::vector<LED> leds;
    std::vector<Camera> cameras;
    
    ceres::Problem problem;
    
    TreeProblem(size_t n_leds, size_t n_cameras)
        :leds(n_leds)
        ,cameras(n_cameras)
    {
        for (size_t i = 0; i < leds.size(); i++) {
            leds[i].original_points.resize(n_cameras);
            leds[i].visible.resize(n_cameras);
        }
    }
    
    void load_points(size_t cam_no, const char *f_name) {
        FILE *in = fopen(f_name, "r");
        
        fscanf(in, "led,x,y,brightness\n");

        while (1) {
            int led_no;
            double x;
            double y;
            double brightness;
            int ret = fscanf(in, "%d,%lf,%lf,%lf\n",
                    &led_no, &x, &y, &brightness);
            if (ret != 4) break;
            
            double x_norm = x - img_size[0] / 2.0;
            double y_norm = y - img_size[1] / 2.0;
            
            leds[led_no].original_points[cam_no] = Point(x, y);
            
            leds[led_no].visible[cam_no] = brightness > 250.0;

            if (leds[led_no].visible[cam_no]) {
                ceres::LossFunction* loss_function = new ceres::HuberLoss(1.0);

                ceres::CostFunction *cf = SnavelyReprojectionError::Create(
                        x_norm,
                        y_norm
                        );

                problem.AddResidualBlock(cf,
                        loss_function,
                        camera_int,
                        cameras[cam_no].camera_ext,
                        leds[led_no].pos);
            }
        }

        fclose(in);
    }
    
    void setup_leds() {
        for (size_t i = 0; i < leds.size(); i++) {
            leds[i].pos[0] = 0.0;
            leds[i].pos[1] = 0.0;
            leds[i].pos[2] = 0.0;
            
            for (size_t j = 0; j < 3; j++) {
                problem.SetParameterLowerBound(leds[i].pos, j, leds[i].pos[j] - 0.5);
                problem.SetParameterUpperBound(leds[i].pos, j, leds[i].pos[j] + 0.5);
            }
        }
    }
    
    void setup_cameras(double pixel_size_mm, double focal_length_mm) {
        double focal_length = focal_length_mm / pixel_size_mm;
        camera_int[0] = focal_length;
        problem.SetParameterLowerBound(camera_int, 0, camera_int[0] * 0.9);
        problem.SetParameterUpperBound(camera_int, 0, camera_int[0] * 1.1);
        
        for (size_t i = 0; i < cameras.size(); i++) {
            double angle = -2.0 * M_PI * (double)i / 8.0;
            cameras[i].camera_ext[0] = 0.0;
            cameras[i].camera_ext[1] = 0.0;
            cameras[i].camera_ext[2] = M_PI/2 + angle;
            
            double r = 1.0;
            cameras[i].camera_ext[3] = r * sin(angle);
            cameras[i].camera_ext[4] = r * cos(angle);
            cameras[i].camera_ext[5] = 0.0;
            problem.SetParameterLowerBound(cameras[i].camera_ext, 5, -0.01);
            problem.SetParameterUpperBound(cameras[i].camera_ext, 5, 0.01);
        }
    }
    
    void run_solve() {
        ceres::Solver::Options options;
        options.linear_solver_type = ceres::DENSE_SCHUR;
        options.minimizer_progress_to_stdout = true;
    
        options.gradient_tolerance = 1e-205;
        options.function_tolerance = 1e-20;
        options.parameter_tolerance = 1e-20;
        options.max_num_iterations = 200;

        ceres::Solver::Summary summary;
        ceres::Solve(options, &problem, &summary);
        std::cout << summary.FullReport() << "\n";
    }
    
    void write_obj(const char *f_name) {
        FILE *out = fopen(f_name, "w");
        
        for (size_t i = 0; i < leds.size(); i++)
            fprintf(out, "v %f %f %f\n", leds[i].pos[0], leds[i].pos[1], leds[i].pos[2]);
        for (size_t i = 0; i < leds.size() - 1; i++)
            fprintf(out, "f %d %d\n", i+1, i+2);
        
        for (size_t i = 0; i < cameras.size(); i++)
            fprintf(out, "v %f %f %f\n", cameras[i].camera_ext[3], cameras[i].camera_ext[4], cameras[i].camera_ext[5]);
        
        fclose(out);
    }
    
    void write_csv(const char *f_name) {
        FILE *out = fopen(f_name, "w");
        
        fprintf(out, "led,x,y,z\n");
        for (size_t i = 0; i < leds.size(); i++)
            fprintf(out, "%d,%f,%f,%f\n", i, leds[i].pos[0], leds[i].pos[1], leds[i].pos[2]);
        
        fclose(out);
    }
    
    void draw_points(size_t cam_no, const char *img_in, const char *img_out)
    {
        Mat im = imread(img_in);
        
        for (size_t i = 0; i < leds.size(); i++) {
            if (!leds[i].visible[cam_no]) continue;
            
            double projected_norm[3];
            project(camera_int,
                    cameras[cam_no].camera_ext,
                    leds[i].pos,
                    projected_norm);
            
            Point projected(
                    projected_norm[0] + img_size[0] / 2.0,
                    projected_norm[1] + img_size[1] / 2.0
                    );
            Point &original = leds[i].original_points[cam_no];
            
            line(im, original, projected, Scalar(0, 255, 0));
        }
        
        imwrite(img_out, im);
    }
};

int main(int argc, char** argv) {
    google::InitGoogleLogging(argv[0]);
    
    TreeProblem p(50, 8);
    p.load_points(0, "points/0.csv");
    p.load_points(1, "points/1.csv");
    p.load_points(2, "points/2.csv");
    p.load_points(3, "points/3.csv");
    p.load_points(4, "points/4.csv");
    p.load_points(5, "points/5.csv");
    p.load_points(6, "points/6.csv");
    p.load_points(7, "points/7.csv");
    
    p.setup_cameras(24.0 / 1920.0, 35.0);
    p.setup_leds();
    
    p.run_solve();
    
    printf("cameras\n");
    for (auto &cam : p.cameras)
        printf("%f %f %f\n", cam.camera_ext[3], cam.camera_ext[4], cam.camera_ext[5]);
    
    printf("\nleds\n");
    for (auto &led : p.leds)
        printf("%f %f %f\n", led.pos[0], led.pos[1], led.pos[2]);
    
    p.write_obj("tmp/out.obj");
    
    p.write_csv("led_positions.csv");
    
    p.draw_points(0, "debug/0/bright.png", "debug/0/projected.png");
    p.draw_points(1, "debug/1/bright.png", "debug/1/projected.png");
    p.draw_points(2, "debug/2/bright.png", "debug/2/projected.png");
    p.draw_points(3, "debug/3/bright.png", "debug/3/projected.png");
    p.draw_points(4, "debug/4/bright.png", "debug/4/projected.png");
    p.draw_points(5, "debug/5/bright.png", "debug/5/projected.png");
    p.draw_points(6, "debug/6/bright.png", "debug/6/projected.png");
    p.draw_points(7, "debug/7/bright.png", "debug/7/projected.png");
}
