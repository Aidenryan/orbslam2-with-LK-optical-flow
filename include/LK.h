//
// Created by lgj on 12/31/19.
//

#ifndef ORB_SLAM2_LK_H
#define ORB_SLAM2_LK_H

#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <chrono>
#include <Frame.h>
using namespace std;
using namespace ORB_SLAM2;


#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/video/tracking.hpp>
using namespace cv;

 vector< cv::Point2f > keypoints;
 vector<cv::Point3f> mappointInCurrentFrame;
vector<cv::Point2f> prev_keypoints;
cv::Mat last_color;
Mat computeMtcwUseLK(KeyFrame *lastKeyFrame, Mat color, bool lastColorIsKeyFrame, Mat K, Mat mDistCoef)
{
    if(last_color.empty())
    {
        cout<<"fill last color fist time"<<endl;
        last_color = color;
        return cv::Mat();
    }
cout<<"TAG0"<<endl;
if(lastColorIsKeyFrame || keypoints.empty())
    {
        cout<<lastKeyFrame->mvKeysUn.size()<<endl;
        keypoints.clear();
        mappointInCurrentFrame.clear();
        cout<<"TAG0.1"<<endl;
        for(int i=0;i<lastKeyFrame->mvpMapPoints.size();i++)//copy point from keyframe
        {
            if(lastKeyFrame->mvpMapPoints[i]&&lastKeyFrame->mvpMapPoints[i]->nObs>1)///if the program died here, try to change 1 to 0
            {
                keypoints.push_back(lastKeyFrame->mvKeysUn[i].pt);
                cv::Point3f pt3f;
                cv::Mat temp;
                temp = lastKeyFrame->mvpMapPoints[i]->GetWorldPos();
                pt3f.x = temp.at<float>(0);
                pt3f.y = temp.at<float>(1);
                pt3f.z = temp.at<float>(2);
                mappointInCurrentFrame.push_back(pt3f);
            }
        }
        cout<<"TAG0.2"<<endl;
    }
    vector<cv::Point2f> next_keypoints;
    prev_keypoints.clear();
    for ( auto kp:keypoints )
        prev_keypoints.push_back(kp);
    vector<unsigned char> status;
    vector<float> error;
    cout<<"TAG1"<<endl;
    if(last_color.empty()||color.empty()||prev_keypoints.empty()||keypoints.empty())//error
    {
        if(last_color.empty())
            cout<<"last color empty"<<endl;
        if(color.empty())
            cout<<"color empty"<<endl;
        if(prev_keypoints.empty())
            cout<<"prev_keypoints empty"<<endl;
        if(keypoints.empty())
            cout<<"keypoint empty"<<endl;
    }
    chrono::steady_clock::time_point t1 = chrono::steady_clock::now();
    cv::calcOpticalFlowPyrLK( last_color, color, prev_keypoints, next_keypoints, status, error );
    chrono::steady_clock::time_point t2 = chrono::steady_clock::now();
    chrono::duration<double> time_used = chrono::duration_cast<chrono::duration<double>>( t2-t1 );
    cout<<"LK Flow use time："<<time_used.count()<<" seconds."<<endl;
    cout<<"TAG2"<<endl;
    // 把跟丢的点删掉
    int i=0;
    for ( auto iter=keypoints.begin(); iter!=keypoints.end(); i++)
    {
        if ( status[i] == 0 )
        {
            iter = keypoints.erase(iter);
            continue;
        }
            *iter = next_keypoints[i];//edit keypoints' coordinate
        iter++;
    }
    cout<<"tracked keypoints: "<<keypoints.size()<<endl;
    i = 0;
    for ( auto iter=mappointInCurrentFrame.begin(); iter!=mappointInCurrentFrame.end(); i++)//erase the match mappoint while the keypoint is erased
    {
        if ( status[i] == 0 )
        {
            iter = mappointInCurrentFrame.erase(iter);
            continue;
        }
        iter++;
    }
    cout<<"TAG3"<<endl;
    vector<cv::Mat> point3D;

    cv:Mat R_vector,T,R;
    if(!(mappointInCurrentFrame.empty()||keypoints.empty()||mDistCoef.empty()))
    {
        chrono::steady_clock::time_point t3 = chrono::steady_clock::now();
        cv::solvePnPRansac(mappointInCurrentFrame, keypoints, K, mDistCoef , R_vector, T);
        cv::Rodrigues(R_vector, R);
        chrono::steady_clock::time_point t4 = chrono::steady_clock::now();
        chrono::duration<double> time_used = chrono::duration_cast<chrono::duration<double>>( t4-t3 );
        cout<<"solve PnP RANSAC use time："<<time_used.count()<<" seconds."<<endl;
        Mat_<double> Rt = (Mat_<double>(4, 4) << R.at<double>(0,0), R.at<double>(0,1), R.at<double>(0,2),T.at<double>(0),
                                                            R.at<double>(1,0), R.at<double>(1,1), R.at<double>(1,2),T.at<double>(1),
                                                            R.at<double>(2,0), R.at<double>(2,1), R.at<double>(2,2),T.at<double>(2),
                                                            0, 0, 0, 1);
        cv::Mat Rt_float;
        Rt.convertTo(Rt_float,CV_32FC1);
        cout<<"LK_PNP_RANSAC pose: "<<endl<<Rt_float<<endl;


    }
    cout<<"TAG4"<<endl;


    if (keypoints.size() == 0)
    {
        cout<<"LK -- all keypoints are lost."<<endl;
        //return cv::Mat();
    }
    // 画出 keypoints
    cv::Mat img_show = color.clone();
    int pointInImg_show = 0;
    for ( auto kp:keypoints )
    {
        pointInImg_show++;
        if(pointInImg_show%4<3)//show a quarter of the points
            continue;
        cv::circle(img_show, kp, 4, cv::Scalar(0, 255, 0), 1);
        cv::circle(img_show, kp, 1, cv::Scalar(0, 255, 0), -1);

    }
    last_color = color;
    return img_show;

}


#endif //ORB_SLAM2_LK_H
