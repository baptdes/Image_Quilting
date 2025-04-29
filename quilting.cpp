#include <opencv2/opencv.hpp>
#include <iostream>
#include <ctime>
#include <vector>
#include <cmath>

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
                cv::Mat left_overlap = output_image(cv::Rect(x_offset - overlap_size, y_offset, overlap_size, actual_height));
                cv::Mat diff;
                cv::absdiff(right_overlap, left_overlap, diff);
                double mse = cv::norm(diff, cv::NORM_L2);
                total_mse += mse * mse;
            }
            
            // Check top (horizontal) overlap if necessary
            if (hasTopOverlap) {
                cv::Mat top_overlap = output_image(cv::Rect(x_offset, y_offset - overlap_size, actual_width, overlap_size));
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

// Find minimum error boundary cut for vertical overlap
void minCutVertical(const cv::Mat& patch, const cv::Mat& output_image, 
                   int x_offset, int y_offset, int overlap_size, 
                   int height, std::vector<int>& cut) {
    
    cv::Mat errors = cv::Mat(height, overlap_size, CV_32F);
    
    // Calculate initial error in the overlap region
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < overlap_size; x++) {
            cv::Vec3b p1 = patch.at<cv::Vec3b>(y, x);
            cv::Vec3b p2 = output_image.at<cv::Vec3b>(y_offset + y, x_offset - overlap_size + x);
            float err = 0;
            for (int c = 0; c < 3; c++) {
                err += std::pow(float(p1[c]) - float(p2[c]), 2);
            }
            errors.at<float>(y, x) = sqrt(err);
        }
    }
    
    // Dynamic programming to find minimal path
    cv::Mat dp = errors.clone();
    for (int y = 1; y < height; y++) {
        for (int x = 0; x < overlap_size; x++) {
            float minVal = dp.at<float>(y-1, x);
            if (x > 0) minVal = std::min(minVal, dp.at<float>(y-1, x-1));
            if (x < overlap_size-1) minVal = std::min(minVal, dp.at<float>(y-1, x+1));
            dp.at<float>(y, x) += minVal;
        }
    }
    
    // Backtrack to find the path
    cut.resize(height);
    int minIdx = 0;
    float minVal = dp.at<float>(height-1, 0);
    for (int x = 1; x < overlap_size; x++) {
        if (dp.at<float>(height-1, x) < minVal) {
            minVal = dp.at<float>(height-1, x);
            minIdx = x;
        }
    }
    cut[height-1] = minIdx;
    
    for (int y = height-2; y >= 0; y--) {
        int x = cut[y+1];
        minIdx = x;
        minVal = dp.at<float>(y, x);
        
        if (x > 0 && dp.at<float>(y, x-1) < minVal) {
            minVal = dp.at<float>(y, x-1);
            minIdx = x-1;
        }
        if (x < overlap_size-1 && dp.at<float>(y, x+1) < minVal) {
            minVal = dp.at<float>(y, x+1);
            minIdx = x+1;
        }
        cut[y] = minIdx;
    }
}

// Find minimum error boundary cut for horizontal overlap
void minCutHorizontal(const cv::Mat& patch, const cv::Mat& output_image, 
                     int x_offset, int y_offset, int overlap_size, 
                     int width, std::vector<int>& cut) {
    
    cv::Mat errors = cv::Mat(overlap_size, width, CV_32F);
    
    // Calculate initial error in the overlap region
    for (int y = 0; y < overlap_size; y++) {
        for (int x = 0; x < width; x++) {
            cv::Vec3b p1 = patch.at<cv::Vec3b>(y, x);
            cv::Vec3b p2 = output_image.at<cv::Vec3b>(y_offset - overlap_size + y, x_offset + x);
            float err = 0;
            for (int c = 0; c < 3; c++) {
                err += std::pow(float(p1[c]) - float(p2[c]), 2);
            }
            errors.at<float>(y, x) = sqrt(err);
        }
    }
    
    // Dynamic programming to find minimal path
    cv::Mat dp = errors.clone();
    for (int x = 1; x < width; x++) {
        for (int y = 0; y < overlap_size; y++) {
            float minVal = dp.at<float>(y, x-1);
            if (y > 0) minVal = std::min(minVal, dp.at<float>(y-1, x-1));
            if (y < overlap_size-1) minVal = std::min(minVal, dp.at<float>(y+1, x-1));
            dp.at<float>(y, x) += minVal;
        }
    }
    
    // Backtrack to find the path
    cut.resize(width);
    int minIdx = 0;
    float minVal = dp.at<float>(0, width-1);
    for (int y = 1; y < overlap_size; y++) {
        if (dp.at<float>(y, width-1) < minVal) {
            minVal = dp.at<float>(y, width-1);
            minIdx = y;
        }
    }
    cut[width-1] = minIdx;
    
    for (int x = width-2; x >= 0; x--) {
        int y = cut[x+1];
        minIdx = y;
        minVal = dp.at<float>(y, x);
        
        if (y > 0 && dp.at<float>(y-1, x) < minVal) {
            minVal = dp.at<float>(y-1, x);
            minIdx = y-1;
        }
        if (y < overlap_size-1 && dp.at<float>(y+1, x) < minVal) {
            minVal = dp.at<float>(y+1, x);
            minIdx = y+1;
        }
        cut[x] = minIdx;
    }
}

// Place patch with minimum error boundary cut
void placePatchWithCut(const cv::Mat& patch, cv::Mat& output_image, 
                      int x_offset, int y_offset, 
                      int actual_width, int actual_height, 
                      int overlap_size) {
    
    bool hasLeftOverlap = (x_offset > 0);
    bool hasTopOverlap = (y_offset > 0);
    
    cv::Mat actual_patch = patch(cv::Rect(0, 0, actual_width, actual_height));
    
    // Create a mask where 1 means use the new patch, 0 means keep existing
    cv::Mat mask = cv::Mat::ones(actual_height, actual_width, CV_8U);
    
    // Handle vertical overlap (left side)
    if (hasLeftOverlap) {
        std::vector<int> cut;
        minCutVertical(patch, output_image, x_offset, y_offset, overlap_size, actual_height, cut);
        
        for (int y = 0; y < actual_height; y++) {
            for (int x = 0; x < overlap_size; x++) {
                if (x < cut[y]) {
                    mask.at<uchar>(y, x) = 0;
                }
            }
        }
    }
    
    // Handle horizontal overlap (top side)
    if (hasTopOverlap) {
        std::vector<int> cut;
        minCutHorizontal(patch, output_image, x_offset, y_offset, overlap_size, actual_width, cut);
        
        for (int x = 0; x < actual_width; x++) {
            for (int y = 0; y < overlap_size; y++) {
                if (y < cut[x]) {
                    mask.at<uchar>(y, x) = 0;
                }
            }
        }
    }
    
    // Apply the mask to blend the patches
    cv::Rect dest_roi(x_offset, y_offset, actual_width, actual_height);
    cv::Mat dest = output_image(dest_roi);
    
    for (int y = 0; y < actual_height; y++) {
        for (int x = 0; x < actual_width; x++) {
            if (mask.at<uchar>(y, x) > 0) {
                dest.at<cv::Vec3b>(y, x) = actual_patch.at<cv::Vec3b>(y, x);
            }
        }
    }
}

int main()
{
    // Initialize random seed
    srand(static_cast<unsigned int>(time(0)));
    
    // Load the texture image
    cv::Mat texture = cv::imread("../Textures/128x128/Brick.0007.ppm", cv::IMREAD_COLOR);
 
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
    int overlap_size = static_cast<int>(block_size * overlap_ratio); // Overlap size

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
            } else {
                // Find the best matching patch
                src_roi = findBestPatch(texture, output_image, x_offset, y_offset, 
                                       block_size, overlap_size, actual_width, actual_height, threshold);
                cv::Mat patch = texture(src_roi).clone();
                
                // Place the patch with minimum error boundary cut
                placePatchWithCut(patch, output_image, x_offset, y_offset, actual_width, actual_height, overlap_size);
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