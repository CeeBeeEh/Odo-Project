#include <fstream>

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <chrono>
#include "odo-lib_opencv_yolo.h"

std::vector<std::string> CLASS_LIST;
cv::dnn::DetectionModel MODEL;
float CONFIDENCE_THRESHOLD;
float NMS_THRESHOLD;
int16_t NET_WIDTH;
int16_t NET_HEIGHT;

std::vector<std::string> getOutputsNames(const cv::dnn::Net& net) {
    static std::vector<std::string> names;
    if (names.empty()) {
        //Get the indices of the output layers, i.e. the layers with unconnected outputs
        std::vector<int> outLayers = net.getUnconnectedOutLayers();

        //get the names of all the layers in the network
        std::vector<std::string> layersNames = net.getLayerNames();

        // Get the names of the output layers in names
        names.resize(outLayers.size());
        for (size_t i = 0; i < outLayers.size(); ++i)
            names[i] = layersNames[outLayers[i] - 1];
    }
    return names;
}

void load_class_list(const char *class_list_path) {
    fprintf(stdout, "LIB: Loading classes list from %s\n", class_list_path);
    std::ifstream ifs(class_list_path);
    std::string line;
    while (getline(ifs, line)) {
        CLASS_LIST.push_back(line);
    }
}

extern "C" void c_odo_load(bool use_cuda, float confidence, float nms, int net_width, int net_height,
                           const char *class_list_path, const char *config_path, const char *weights_path) {
    odo_load(use_cuda, confidence, nms, net_width, net_height, class_list_path, config_path, weights_path);
}

void odo_load(bool use_cuda, float confidence, float nms, int net_width, int net_height,
              const char *class_list_path, const char *config_path, const char *weights_path) {
    fprintf(stdout, "LIB: Entering load\n");
    CONFIDENCE_THRESHOLD = confidence;
    NMS_THRESHOLD = nms;
    NET_WIDTH = static_cast<int16_t>(net_width);
    NET_HEIGHT = static_cast<int16_t>(net_height);
    fprintf(stdout, "LIB: Thresholds - Confidence=%f - NMS=%f\n", CONFIDENCE_THRESHOLD, NMS_THRESHOLD);
    load_class_list(class_list_path);
    fprintf(stdout, "LIB: Loading darknet config from %s\n", config_path);
    fprintf(stdout, "LIB: Loading darknet weights from %s\n", weights_path);
    auto net = cv::dnn::readNetFromDarknet(config_path, weights_path);
    fprintf(stdout, "LIB: Setting backend\n");
    if (use_cuda) {
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
    } else {
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    }
    MODEL = cv::dnn::DetectionModel(net);
    MODEL.setInputParams(1/255.0, cv::Size(NET_WIDTH, NET_HEIGHT), cv::Scalar(), true);
    fprintf(stdout, "LIB: Loading complete\n");
}

extern "C" void c_odo_free() {
    odo_free();
}

void odo_free() {
    // nothing to do here
}

extern "C" void c_odo_detect(ulong &num, DetectionData *detectData, unsigned char *img,
                             size_t img_format, int img_width, int img_height) {
    return odo_detect(num, detectData, img, img_format, img_width, img_height);
}

void odo_detect(ulong &num, DetectionData *detectData, unsigned char *img,
                size_t img_format, int img_width, int img_height) {
    cv::Mat frame, fixed, blob;
    std::vector<cv::Mat> detections;
    int imgType = CV_8UC3;
    bool rbSwap = false;

    switch (img_format) {
        case 11:
            rbSwap = true;
            imgType = CV_8UC4;
            break;
        case 12:
            imgType = CV_8UC4;
            break;
        case 15:
            rbSwap = true;
            imgType = CV_8UC3;
            break;
        case 16:
            imgType = CV_8UC3;
            break;
        default:
            fprintf(stderr, "LIB: Unsupported image format\n");
            num = -1;
            return;
    }

    if (img == nullptr) {
        fprintf(stderr, "LIB: Image is null\n");
        num = -1;
        return;
    }

    try {
        //auto start = std::chrono::high_resolution_clock::now();
        try {
            frame = cv::Mat(cv::Size(img_width, img_height), imgType, img);
        }
        catch (cv::Exception &e) {
            fprintf(stderr, "LIB: Error in forward pass: %s\n", e.what());
            num = -1;
            return;
        }

        std::vector<int> classIds;
        std::vector<float> scores;
        std::vector<cv::Rect> boxes;
        MODEL.detect(frame, classIds, scores, boxes, CONFIDENCE_THRESHOLD, NMS_THRESHOLD);

        num = boxes.size();

        for (ulong i = 0; i < num; ++i) {
            if (scores[i] < CONFIDENCE_THRESHOLD) continue;
            auto box = boxes[i];
            auto classId = classIds[i];
            DetectionData detection;
            detection.box = xyToBox(box.x, box.y, box.width, box.height);
            strcpy(detection.label, CLASS_LIST.at(classId).c_str());
            detection.class_id = classId;
            detection.confidence = scores[i];
            detectData[i] = detection;
        }
    }
    catch (const std::exception &e) {
        fprintf(stderr, "LIB: Error running detection\n");
    }
}