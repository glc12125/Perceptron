#include "camerathread.h"
#include "utils.h"



CameraTask::CameraTask(BetterVideoCapture *camera, QVideoFrame *videoFrame, unsigned char *cvImageBuf, int width, int height)
{
    this->running = true;
    this->camera = camera;
    this->videoFrame = videoFrame;
    this->cvImageBuf = cvImageBuf;
    this->width = width;
    this->height = height;

}

CameraTask::~CameraTask()
{
    //Leave camera and videoFrame alone, they will be destroyed elsewhere
}

void CameraTask::stop()
{
    running = false;
}

void CameraTask::doWork()
{
    running = true;
    if(videoFrame)
        videoFrame->map(QAbstractVideoBuffer::ReadOnly);

    cv::Mat screenImage;
    if(videoFrame)
        screenImage = cv::Mat(height,width,CV_8UC4,videoFrame->bits());

    int x = 80;
    int y = 150;
    cv::RNG rng(time(0));
    QMap< unsigned int, cv::Scalar > colors;
    for(int i=0; i < 80; i++)
    {
        cv::Scalar color = cv::Scalar(rng.uniform(0,255),rng.uniform(0,255),rng.uniform(0,255));
        colors.insert(i, color);
    }


    while(running && videoFrame != NULL && camera != NULL)
    {
        if(x < 150)
            x += 14;
        else
            x = 80;
        if(y < 250)
            y += 14;
        else
            y = 150;

        if(!camera->grabFrame())
            continue;
        unsigned char* cameraFrame = camera->retrieveFrame();

        //Get camera image into screen frame buffer
        if(videoFrame)
        {
            // Assuming desktop, RGB camera image and RGBA QVideoFrame
            cv::Mat tempMat(height, width, CV_8UC3, cameraFrame);

            std::shared_ptr<image_t> img = detector->mat_to_image(tempMat);
            image_t *iii = img.get();
            std::vector<bbox_t> results;
            results = detector->detect(*iii);

            qDebug() << "Person numbers: " << results.size();
            for(size_t i = 0; i < results.size(); i++)
            {
                qDebug() << "[" << i << "] " << results[i].x << " " << results[i].y << " " << results[i].w << " " << results[i].h;
                cv::Rect rec(results[i].x, results[i].y, results[i].w, results[i].h);

                cv::rectangle(tempMat, rec, colors.find(results[i].obj_id).value(), 4);
            }

            cv::putText(tempMat, "FUCK THE WORLD", cv::Point(x, y), cv::FONT_HERSHEY_SIMPLEX, 1.6, cv::Scalar(rng.uniform(0,255),rng.uniform(0,255),rng.uniform(0,255)), 8);

            cv::cvtColor(tempMat, screenImage, cv::COLOR_RGB2RGBA);
        }

        if(cvImageBuf)
        {
            // Assuming desktop, RGB camera image
            memcpy(cvImageBuf, cameraFrame, height*width*3);
        }

        emit imageReady();
    }

}


CameraThread::CameraThread(BetterVideoCapture *camera, QVideoFrame *videoFrame, unsigned char *cvImageBuf, int width, int height)
{
    task = new CameraTask(camera,videoFrame,cvImageBuf,width,height);
    task->moveToThread(&workerThread);
    connect(&workerThread, SIGNAL(started()), task, SLOT(doWork()));
    connect(task, SIGNAL(imageReady()), this, SIGNAL(imageReady()));
}

CameraThread::~CameraThread()
{
    stop();
    delete task;
}

void CameraThread::start()
{
    if(!workerThread.isRunning())
        workerThread.start();
    else
    {
        task->doWork();
    }
}

void CameraThread::stop()
{
    if(task != NULL)
        task->stop();
    workerThread.quit();
    workerThread.wait();
}













