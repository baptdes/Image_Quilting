#include <opencv2/opencv.hpp>
#include<iostream>

cv::Rect getRandomPatch(const cv::Mat& image, int patch_size) {
    int x = rand() % (image.cols - patch_size);
    int y = rand() % (image.rows - patch_size);
    return cv::Rect(y, x, patch_size, patch_size);
}

int main()
{
    cv::Mat texture = imread("../Textures/128x128/Brick.0007.ppm", cv::IMREAD_COLOR);
 
    if (texture.empty()) {
        std::cout << "Error: Texture not found or unable to read." << std::endl;
    } else {
        std::cout << "Texture loaded successfully!" << std::endl;
    }
 
    cv::imshow("Texture", texture);

    // Define the size of the patch
    int block_size = 60;

    // Output image
    int output_width = 300;
    int output_height = 400;

    // Compute the number of patches
    int num_patches_width = (output_width + block_size - 1) / block_size;
    int num_patches_height = (output_height + block_size - 1) / block_size;

    // Block image
    cv::Mat output_image(num_patches_height * block_size, num_patches_width * block_size, CV_8UC3, cv::Scalar(0, 0, 0));

    // Loop to fill the output image with patches
    for (int y = 0; y < num_patches_height; ++y) {
        for (int x = 0; x < num_patches_width; ++x) {
            // Randomly select a patch from the texture
            cv::Rect roi = getRandomPatch(texture, block_size);
            cv::Mat patch = texture(roi);

            // Place the patch in the output image
            cv::Rect output_roi(x * block_size, y * block_size, block_size, block_size);
            patch.copyTo(output_image(output_roi));
        }
    }

    // Crop the output image to the desired size
    cv::Rect crop_roi(0, 0, output_width, output_height);
    output_image = output_image(crop_roi);

    // Display the output image
    cv::imshow("Block Image", output_image);
    
    cv::waitKey(0);
    cv::destroyAllWindows();
    return 0;
}