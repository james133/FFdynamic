#include <string>
#include "testCommon.h"
#include "cvDnnDetect.h"
#include "cvPostDraw.h"
#include "cvStreamlet.h"

using namespace ff_dynamic;

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "usage: testDehazor inputFile\n";
        return -1;
    }

    test_common::testInit(argv[0]);
    LOG(INFO) << "starting test: " << argv[1];
    // 1. create demux and get stream info
    DavWaveOption demuxOption((DavWaveClassDemux()));
    demuxOption.set(DavOptionInputUrl(), argv[1]);
    // 2. video/audio decode
    DavWaveOption videoDecodeOption((DavWaveClassVideoDecode()));
    DavWaveOption audioDecodeOption((DavWaveClassAudioDecode()));
    // 3. video encode
    DavWaveOption videoEncodeOption((DavWaveClassVideoEncode()));
    videoEncodeOption.setVideoSize(1280, 720);
    videoEncodeOption.set("preset", "veryfast");
    // 4. audio encode
    DavWaveOption audioEncodeOption((DavWaveClassAudioEncode()));
    // 5. mux
    DavWaveOption muxOption((DavWaveClassMux()));
    muxOption.set(DavOptionOutputUrl(), "dynaDetect.flv");

    // 6. dynamic dnn detection streamlet
    DavWaveOption dataRelayOption((DavWaveClassDataRelay()));
    DavWaveOption cvPostDrawOption((DavWaveClassCvPostDraw()));

    // we craete two dnn detectors
    DavWaveOption cvDnnDetectOption1((DavWaveClassCvDnnDetect()));
    cvDnnDetectOption1.set(DavOptionImplType(), "auto");
    cvDnnDetectOption1.set("detector_type", "detect");
    cvDnnDetectOption1.set("detector_framework_tag", "darknet/yolov3");
    cvDnnDetectOption1.set("model_path", "yolov3.weights");
    cvDnnDetectOption1.set("config_path", "yolov3.cfg");
    cvDnnDetectOption1.set("classname_path", "coco.names");
    cvDnnDetectOption1.setInt("backend_id", 3);
    cvDnnDetectOption1.setInt("target_id", 0);
    cvDnnDetectOption1.setDouble("scale_factor", 1.0/255.0);
    cvDnnDetectOption1.setBool("swap_rb", true);
    cvDnnDetectOption1.setInt("width", 416);
    cvDnnDetectOption1.setInt("height", 416);
    cvDnnDetectOption1.setDouble("conf_threshold", 0.7);
    cvDnnDetectOption1.setDoubleArray("means", {0,0,0});

    DavWaveOption cvDnnDetectOption2((DavWaveClassCvDnnDetect()));
    cvDnnDetectOption2.set(DavOptionImplType(), "auto");
    cvDnnDetectOption2.set("detector_type", "detect");
    cvDnnDetectOption2.set("detector_framework_tag", "caffemodel/vgg_ssd_512");
    cvDnnDetectOption2.set("model_path", "VGG_VOC0712Plus_SSD_512x512_ft_iter_160000.caffemodel");
    cvDnnDetectOption2.set("config_path", "vgg_ssd_512.prototxt");
    cvDnnDetectOption2.set("classname_path", ""); // not needed
    cvDnnDetectOption2.setInt("backend_id", 3);
    cvDnnDetectOption2.setInt("target_id", 0);
    cvDnnDetectOption2.setDouble("scale_factor", 1.0/255.0);
    cvDnnDetectOption2.setBool("swap_rb", true);
    cvDnnDetectOption2.setInt("width", 512);
    cvDnnDetectOption2.setInt("height", 512);
    cvDnnDetectOption2.setDouble("conf_threshold", 0.7);
    cvDnnDetectOption2.setDoubleArray("means", {0,0,0});

    ////////////////////////////////////////////////////////////////////////////
    DavDefaultInputStreamletBuilder inputBuilder;
    DavDefaultOutputStreamletBuilder outputBuilder;
    CvDnnDetectStreamletBuilder cvDnnBuilder;
    DavStreamletOption inputOption;
    DavStreamletOption outputOption;
    DavStreamletOption cvDnnBuildOption;
    // start build
    inputOption.setInt(DavOptionBufLimitNum(), 25);
    auto streamletInput = inputBuilder.build({demuxOption, videoDecodeOption, audioDecodeOption},
                                             DavDefaultInputStreamletTag("input"), inputOption);
    CHECK(streamletInput != nullptr);
    auto streamletOutput = outputBuilder.build({videoEncodeOption, audioEncodeOption, muxOption},
                                               DavDefaultOutputStreamletTag("output"));
    CHECK(streamletOutput != nullptr);
    auto cvDnnStreamlet = cvDnnBuilder.build({dataRelayOption, cvPostDrawOption, cvDnnDetectOption1, cvDnnDetectOption2},
                                              CvDnnDetectStreamletTag("cvDnn"), cvDnnBuildOption);
    /* connect streamlets */
    streamletInput >> cvDnnStreamlet >> streamletOutput;

    // start
    DavRiver river({streamletInput, cvDnnStreamlet, streamletOutput});
    river.start();
    test_common::testRun(river);
    river.stop();
    river.clear();
    return 0;
}