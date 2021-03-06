#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/timer/timer.hpp>

#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <mtcnn/detector.h>

namespace fs = boost::filesystem;

using rectPoints = std::pair<cv::Rect, std::vector<cv::Point>>;

static cv::Mat drawRectsAndPoints(const cv::Mat &img,
                                  const std::vector<rectPoints> data)
{
  cv::Mat outImg;
  img.convertTo(outImg, CV_8UC3);

  for (auto &d : data)
  {
    cv::rectangle(outImg, d.first, cv::Scalar(0, 0, 255));
    auto pts = d.second;
    for (size_t i = 0; i < pts.size(); ++i)
    {
      cv::circle(outImg, pts[i], 3, cv::Scalar(0, 0, 255));
      cv::putText(
                outImg, std::to_string(i), pts[i], cv::FONT_HERSHEY_DUPLEX, 0.4, {0,0,255}, 1, cv::LINE_AA);
    }
  }
  return outImg;
}

int main(int argc, char **argv)
{

  if (argc < 3)
  {
    std::cerr << "Usage " << argv[0] << ": "
              << "<model-dir> "
              << " "
              << "<test-image>\n";
    return -1;
  }

  fs::path modelDir = fs::path(argv[1]);

  ProposalNetwork::Config pConfig;
  pConfig.caffeModel = (modelDir / "det1.caffemodel").string();
  pConfig.protoText = (modelDir / "det1.prototxt").string();
  pConfig.threshold = 0.6f;

  RefineNetwork::Config rConfig;
  rConfig.caffeModel = (modelDir / "det2.caffemodel").string();
  rConfig.protoText = (modelDir / "det2.prototxt").string();
  rConfig.threshold = 0.7f;

  OutputNetwork::Config oConfig;
  oConfig.caffeModel = (modelDir / "det3.caffemodel").string();
  oConfig.protoText = (modelDir / "det3.prototxt").string();
  oConfig.threshold = 0.7f;

  MTCNNDetector detector(pConfig, rConfig, oConfig);

  cv::VideoCapture cap;
  cap.open(1);
  cv::Mat img; // = cv::imread(argv[2]);
  if (!cap.isOpened()) {
    return -1;
  }
  while (1)
  {
    cap >> img;
    std::vector<Face> faces;

    {
      boost::timer::auto_cpu_timer t(3, "%w seconds\n");
      faces = detector.detect(img, 20.f, 0.709f);
    }

    std::cout << "Number of faces found in the supplied image - " << faces.size()
              << std::endl;

    std::vector<rectPoints> data;

    // show the image with faces in it
    for (size_t i = 0; i < faces.size(); ++i)
    {
      std::vector<cv::Point> pts;
      for (int p = 0; p < NUM_PTS; ++p)
      {
        pts.push_back(
            cv::Point(faces[i].ptsCoords[2 * p], faces[i].ptsCoords[2 * p + 1]));
      }

      auto rect = faces[i].bbox.getRect();
      auto d = std::make_pair(rect, pts);
      data.push_back(d);
    }

    auto resultImg = drawRectsAndPoints(img, data);
    cv::imshow("test-oc", resultImg);
    int x = cv::waitKey(5);
    if (x == 'x') {
      return 0;
    }
  }
  return 0;
}
