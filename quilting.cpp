#include <opencv2/opencv.hpp>
#include <iostream>
#include <ctime>
#include <vector>
#include <cmath>
#include <algorithm>

// Get a random patch from the input texture
cv::Rect getRandomPatch(const cv::Mat& image, int patch_size) {
    int x = rand() % (image.cols - patch_size);
    int y = rand() % (image.rows - patch_size);
    return cv::Rect(x, y, patch_size, patch_size);
}

// Find the best matching patch based on overlap regions
cv::Rect findBestPatch(const cv::Mat& texture, const cv::Mat& output_image, 
                      int x_offset, int y_offset, 
                      int block_size, int overlap_size,
                      int actual_width, int actual_height, 
                      double threshold) {
    
    std::vector<double> mse_values;
    std::vector<cv::Rect> coords;
    
    bool hasLeftOverlap = (x_offset > 0);
    bool hasTopOverlap = (y_offset > 0);
    
    // Scan through all possible patches in the texture
    for (int i = 0; i <= texture.rows - block_size; ++i) {
        for (int j = 0; j <= texture.cols - block_size; ++j) {
            double total_mse = 0.0;
            
            // Check left (vertical) overlap if necessary
            if (hasLeftOverlap) {
                cv::Mat right_overlap = texture(cv::Rect(j, i, overlap_size, actual_height));
                cv::Mat left_overlap = output_image(cv::Rect(x_offset, y_offset, overlap_size, actual_height));
                cv::Mat diff;
                cv::absdiff(right_overlap, left_overlap, diff);
                double mse = cv::norm(diff, cv::NORM_L2);
                total_mse += mse * mse;
            }
            
            // Check top (horizontal) overlap if necessary
            if (hasTopOverlap) {
                cv::Mat top_overlap = output_image(cv::Rect(x_offset, y_offset, actual_width, overlap_size));
                cv::Mat bottom_overlap = texture(cv::Rect(j, i, actual_width, overlap_size));
                cv::Mat diff;
                cv::absdiff(top_overlap, bottom_overlap, diff);
                double mse = cv::norm(diff, cv::NORM_L2);
                total_mse += mse * mse;
            }
            
            if (hasLeftOverlap || hasTopOverlap) {
                total_mse = sqrt(total_mse);
                mse_values.push_back(total_mse);
                coords.push_back(cv::Rect(j, i, block_size, block_size));
            }
        }
    }
    
    // Find the best coordinates within tolerance
    if (mse_values.empty()) {
        return getRandomPatch(texture, block_size);
    }
    
    double min_error = *std::min_element(mse_values.begin(), mse_values.end());
    std::vector<cv::Rect> good_coords;
    
    for (size_t i = 0; i < mse_values.size(); ++i) {
        if (mse_values[i] <= min_error * (1.0 + threshold)) {
            good_coords.push_back(coords[i]);
        }
    }
    
    // Select a random patch from the good coordinates
    size_t random_index = static_cast<size_t>(rand()) % good_coords.size();
    return good_coords[random_index];
}


cv::Mat computeVerticalErrorMap(const cv::Mat& overlap1, const cv::Mat& overlap2) {
    cv::Mat diff;
    cv::absdiff(overlap1, overlap2, diff);
    
    // Create the initial cost map
    cv::Mat cost_map = cv::Mat(diff.rows, diff.cols, CV_64F);
    for (int i = 0; i < diff.rows; ++i) {
        for (int j = 0; j < diff.cols; ++j) {
            double sum = 0;
            for (int c = 0; c < diff.channels(); ++c) {
                double val = diff.at<cv::Vec3b>(i, j)[c];
                sum += val * val;
            }
            cost_map.at<double>(i, j) = sum/diff.channels();
        }
    }
    
    for (int i = 1; i < cost_map.rows; ++i) {
        for (int j = 0; j < cost_map.cols; ++j) {
            double min_cost = cost_map.at<double>(i-1, j); // Top
            
            if (j > 0)
                min_cost = std::min(min_cost, cost_map.at<double>(i-1, j-1)); // Top-left
                
            if (j < cost_map.cols - 1)
                min_cost = std::min(min_cost, cost_map.at<double>(i-1, j+1)); // Top-right
                
            cost_map.at<double>(i, j) += min_cost;
        }
    }
    return cost_map;
}

cv::Mat computeHorizontalErrorMap(const cv::Mat& overlap1, const cv::Mat& overlap2) {
    cv::Mat diff;
    cv::absdiff(overlap1, overlap2, diff);
    
    // Create the initial cost map
    cv::Mat cost_map = cv::Mat(diff.rows, diff.cols, CV_64F);
    for (int i = 0; i < diff.rows; ++i) {
        for (int j = 0; j < diff.cols; ++j) {
            double sum = 0;
            for (int c = 0; c < diff.channels(); ++c) {
                double val = diff.at<cv::Vec3b>(i, j)[c];
                sum += val * val;
            }
            // Average the squared differences between channels
            cost_map.at<double>(i, j) = sum/diff.channels();
        }
    }

    for (int j = cost_map.cols - 2; j >= 0; --j) {
        for (int i = 0; i < cost_map.rows; ++i) {
            double min_cost = cost_map.at<double>(i, j+1); // Right
            
            if (i > 0)
                min_cost = std::min(min_cost, cost_map.at<double>(i-1, j+1)); // Top-right
                
            if (i < cost_map.rows - 1)
                min_cost = std::min(min_cost, cost_map.at<double>(i+1, j+1)); // Bottom-right
                
            cost_map.at<double>(i, j) += min_cost;
        }
    }
    return cost_map;
}


int main()
{
    // Initialize random seed
    srand(static_cast<unsigned int>(time(0)));
    
    // Load the texture image
    cv::Mat texture = cv::imread("../Textures/128x128/Food.0006.ppm", cv::IMREAD_COLOR);
 
    if (texture.empty()) {
        std::cout << "Error: Texture not found or unable to read." << std::endl;
        return -1;
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
    int overlap_size = static_cast<int>(static_cast<float>(block_size) * overlap_ratio); // Overlap size

    // Compute the number of patches
    int num_patches_width = 1 + static_cast<int>(std::ceil((output_width - block_size) * 1.0 / (block_size - overlap_size)));
    int num_patches_height = 1 + static_cast<int>(std::ceil((output_height - block_size) * 1.0 / (block_size - overlap_size)));

    // Create output image
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

            cv::Rect src_roi;
            
            if (x == 0 && y == 0) {
                // First patch, just place randomly
                src_roi = getRandomPatch(texture, block_size);
                cv::Mat patch = texture(src_roi).clone();
                cv::Rect coord_current_patch(x_offset, y_offset, actual_width, actual_height);
                patch(cv::Rect(0, 0, actual_width, actual_height)).copyTo(output_image(coord_current_patch));
            } else if (x == 0) {
                // First column, only check top overlap

                // Find the best matching patch
                src_roi = findBestPatch(texture, output_image, x_offset, y_offset, 
                                       block_size, overlap_size, actual_width, actual_height, threshold);

                // Compute the error surface
                cv::Mat top_overlap = output_image(cv::Rect(x_offset, y_offset, actual_width, overlap_size));
                cv::Mat bottom_overlap = texture(cv::Rect(src_roi.x, src_roi.y, actual_width, overlap_size));
                cv::Mat error_map = computeHorizontalErrorMap(top_overlap, bottom_overlap);

                // Find the minimum cost path along this surface
                double min_cost = std::numeric_limits<double>::max();
                // Array of minimum paths
                std::vector<int> min_paths(error_map.cols, 0.0);
                for (int i = 0; i < error_map.rows; ++i) {
                    if (error_map.at<double>(i, 0) < min_cost) {
                        min_cost = error_map.at<double>(i, 0);
                        min_paths[i] = i;
                    }
                }
                // Complete the path
                for (int j = 1; j < error_map.cols; ++j) {
                    int ind_ligne = min_paths[j - 1];
                    if (ind_ligne > 0 && ind_ligne < error_map.rows - 1 && error_map.at<double>(ind_ligne - 1, j) < error_map.at<double>(ind_ligne, j) && error_map.at<double>(ind_ligne - 1, j) < error_map.at<double>(ind_ligne + 1, j)) {
                        min_paths[j] = ind_ligne - 1;
                    } else if (ind_ligne < error_map.rows - 1 && error_map.at<double>(ind_ligne + 1, j) < error_map.at<double>(ind_ligne, j)) {
                        min_paths[j] = ind_ligne + 1;
                    } else {
                        min_paths[j] = ind_ligne;
                    }
                }

                // Paste the patch following the path
                cv::Mat patch = texture(src_roi).clone();
                for (int j = 0; j < error_map.cols; ++j) {
                    int y_coord = min_paths[j];
                    for (int i = 0; i < y_coord; ++i) {
                        patch.at<cv::Vec3b>(i, j) = output_image.at<cv::Vec3b>(y_offset + i, x_offset + j);
                    }
                    //Mettre un pixel rouge à la frontière
                    //patch.at<cv::Vec3b>(y_coord, j) = cv::Vec3b(0, 0, 255);
                }

                cv::Rect coord_current_patch(x_offset, y_offset, actual_width, actual_height);
                patch(cv::Rect(0, 0, actual_width, actual_height)).copyTo(output_image(coord_current_patch));
                
            } else {
                // Find the best matching patch
                src_roi = findBestPatch(texture, output_image, x_offset, y_offset, 
                                       block_size, overlap_size, actual_width, actual_height, threshold);
                cv::Mat patch = texture(src_roi).clone();

                cv::Rect coord_current_patch(x_offset, y_offset, actual_width, actual_height);
                patch(cv::Rect(0, 0, actual_width, actual_height)).copyTo(output_image(coord_current_patch));
            }
        }
    }

    // Display the output image
    cv::imshow("Quilted Image", output_image);
    
    // Save the output image
    cv::imwrite("quilted_output.png", output_image);
    std::cout << "Output image saved as quilted_output.png" << std::endl;
    
    cv::waitKey(0);
    cv::destroyAllWindows();
    return 0;
}