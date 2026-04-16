#include "camera.hpp"

int saveImageFromeCamera(std::string file, int camera){
	cv::Mat frame;
	cv::VideoCapture cap;
	cap.open(camera);
	if (!cap.isOpened()) {
		std::cerr << "ERROR! Unable to open camera\n";
        	return -1;
    	}
        // wait for a new frame from camera and store it into 'frame'
        cap.read(frame);
        // check if we succeeded
        if (frame.empty()) {
		std::cerr << "ERROR! blank frame grabbed\n";
        	
	}
	
        // show live and wait for a key with timeout long enough to show images
	cv::imwrite(file, frame);
	return 0;
}

