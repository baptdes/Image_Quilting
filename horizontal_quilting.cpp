#include <opencv2/opencv.hpp>
#include<iostream>

cv::Rect getRandomPatch(const cv::Mat& image, int patch_size) {
    int x = rand() % (image.cols - patch_size);
    int y = rand() % (image.rows - patch_size);
    return cv::Rect(x, y, patch_size, patch_size);
}

int main()
{
    cv::Mat texture = imread("../Textures/128x128/Food.0006.ppm", cv::IMREAD_COLOR);
 
    if (texture.empty()) {
        std::cout << "Error: Texture not found or unable to read." << std::endl;
    } else {
        std::cout << "Texture loaded successfully!" << std::endl;
    }
 
    cv::imshow("Texture", texture);

    /* ||||||||||||||||||||| Parameters |||||||||||||||||||||| */
    int block_size = 60; // Define the size of the patch
    // Output image
    int output_width = 300;
    int output_height = 400;

    float overlap_ratio = 1.0f/6.0f; // Overlap ratio

    double threshold = 0.1; // Threshold for similarity

    /* |||||||||||||||||| Texture synthesis |||||||||||||||||||*/
    int overlap_size = static_cast<int>(1.0 * block_size * overlap_ratio); // Overlap size

    // Compute the number of patches
    int num_patches_width = 1 + static_cast<int>(std::ceil(1.0 * (output_width - block_size) / (block_size - overlap_size)));
    int num_patches_height = 1 + static_cast<int>(std::ceil(1.0 * (output_height - block_size) / (block_size - overlap_size)));

    // Block image
    cv::Mat output_image(output_height, output_width, CV_8UC3, cv::Scalar(0, 0, 0));

    // Loop to fill the output image with patches
    for (int y = 0; y < num_patches_height; ++y) {
        for (int x = 0; x < num_patches_width; ++x) {
            // Coordinates for the patch in the output image
            int x_offset = x * (block_size - overlap_size);
            int y_offset = y * (block_size - overlap_size);
            
            // Calculate the actual size of the patch to place (handles image boundaries)
            int actual_width = std::min(block_size, output_width - x_offset);
            int actual_height = std::min(block_size, output_height - y_offset);

            if (x == 0) {
                // First patch, no overlap
                cv::Rect src_roi = getRandomPatch(texture, block_size);
                cv::Mat patch = texture(src_roi).clone();
                cv::Rect coord_current_patch(x_offset, y_offset, actual_width, actual_height);
                
                // Copy the patch the output image
                patch(cv::Rect(0, 0, actual_width, actual_height)).copyTo(output_image(coord_current_patch));
            } else {
                // List of error values
                std::vector<double> mse_values;
                // List of coordinates
                std::vector<cv::Rect> coords;

                // Scan for coordinates
                for (int i = 0; i < texture.rows - block_size; ++i) {
                    for (int j = 0; j < texture.cols - block_size; ++j) {
                        cv::Mat right_overlap = texture(cv::Rect(j, i, overlap_size, actual_height));
                        cv::Mat left_overlap = output_image(cv::Rect(x_offset, y_offset, overlap_size, actual_height));

                        // Check if the overlaps are similar (Mean square error)
                        cv::Mat diff;
                        cv::absdiff(right_overlap, left_overlap, diff);
                        double mse = cv::norm(diff, cv::NORM_L2);
                        mse_values.push_back(mse);
                        coords.push_back(cv::Rect(j, i, block_size, block_size));
                    }
                }

                // Find the best coordinates (within 0.1 times the error of the best)
                double min_error = *std::min_element(mse_values.begin(), mse_values.end());
                printf("Minimum error: %f\n", min_error);
                std::vector<cv::Rect> good_coords;
                for (long unsigned int i = 0; i < mse_values.size(); ++i) {
                    if (mse_values[i] <= min_error * (1.0 + threshold)) {
                        good_coords.push_back(coords[i]);
                    }
                }
                printf("Number of good coordinates: %lu\n", good_coords.size());
                // Select a random patch from the good coordinates
                long unsigned int random_index = static_cast<long unsigned int>(rand()) % good_coords.size();
                cv::Rect src_roi = good_coords[random_index];
                cv::Mat patch = texture(src_roi).clone();
                // Define the destination in the output image
                cv::Rect coord_current_patch(x_offset, y_offset, actual_width, actual_height);
                
                // Copy the patch to the output image
                patch(cv::Rect(0, 0, actual_width, actual_height)).copyTo(output_image(coord_current_patch));
            }
        }
    }

    // Display the output image
    cv::imshow("Block Image", output_image);
    
    cv::waitKey(0);
    cv::destroyAllWindows();
    return 0;
}