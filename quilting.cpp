#include <opencv2/opencv.hpp>
#include<iostream>

int main()
{
    cv::Mat texture = imread("/home/nimdaba/Documents/N7/2A/S8/Projet_Image/Textures/128x128/Bark.0000.ppm", cv::IMREAD_COLOR);
 
    if (texture.empty()) {
        std::cout << "Error: Texture not found or unable to read." << std::endl;
    } else {
        std::cout << "Texture loaded successfully!" << std::endl;
    }
 
    cv::imshow("Texture", texture);
    
    cv::waitKey(0);
    cv::destroyAllWindows();
    return 0;
}