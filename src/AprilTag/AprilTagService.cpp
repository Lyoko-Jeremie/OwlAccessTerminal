//jeremie

#include <boost/asio/strand.hpp>
#include "AprilTagService.h"


extern "C" {
#include "apriltag.h"
#include "tag36h11.h"
#include "tag25h9.h"
#include "tag16h5.h"
#include "tagCircle21h7.h"
#include "tagCircle49h12.h"
#include "tagCustom48h12.h"
#include "tagStandard41h12.h"
#include "tagStandard52h13.h"
#include "common/getopt.h"
}


namespace OwlAprilTagService {

    struct AprilTagData : public std::enable_shared_from_this<AprilTagData> {
        explicit AprilTagData(
                boost::asio::io_context &ioc
        ) : strand_(boost::asio::make_strand(ioc)) {
            td = apriltag_detector_create();
            tf = tagStandard41h12_create();
            apriltag_detector_add_family(td, tf);
        }


        ~AprilTagData() {
            // Cleanup.
            tagStandard41h12_destroy(tf);
            apriltag_detector_destroy(td);
        }

        boost::asio::strand<boost::asio::io_context::executor_type> strand_;
        apriltag_detector_t *td;
        apriltag_family_t *tf;
    };


    void AprilTagService::calcTag(const cv::Mat& image) {
        // TODO calc it and call the `sendMailCmd` to send info to airplane controller
        boost::asio::dispatch(AprilTagData_->strand_, [this, self = shared_from_this(), image]
                () {

            image_u8_t img_header = {.width = image.cols,
                    .height = image.rows,
                    .stride = image.cols,
                    .buf = image.data
            };

            zarray_t *detections = apriltag_detector_detect(AprilTagData_->td, &img_header);

            for (int i = 0; i < zarray_size(detections); i++) {
                apriltag_detection_t *det;
                zarray_get(detections, i, &det);

                // TODO Do stuff with detections here.

            }

        });
    }
} // OwlAprilTagService