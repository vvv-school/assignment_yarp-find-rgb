/*
 * Copyright (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Giulia Vezzani <giulia.vezzani@iit.it>
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
*/

#include <vector>
#include <deque>
#include <cmath>
#include <array>
#include <algorithm>
#include <utility>

#include <rtf/dll/Plugin.h>
#include <rtf/TestAssert.h>

#include <yarp/rtf/TestCase.h>
#include <yarp/os/all.h>
#include <yarp/dev/all.h>
#include <yarp/sig/all.h>
#include <yarp/math/Math.h>
#include <yarp/math/Rand.h>

#include <iCub/ctrl/math.h>

using namespace std;
using namespace RTF;
using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::sig;
using namespace yarp::math;
using namespace iCub::ctrl;

class TestAssignmentYarpFindRgb : public yarp::rtf::TestCase
{
    PolyDriver                       driver;
    IEncoders                       *ienc;

    BufferedPort<Bottle>             anglePort;
    BufferedPort<Bottle>             colorPort;
    RpcClient                        ballPort;

    Vector                           ball_col;
    Vector                           ball_pos;

    /****************************************************/
    void createBall()
    {
        Bottle cmd,reply;
        cmd.addString("world");
        cmd.addString("mk");
        cmd.addString("ssph");
        cmd.addDouble(0.06);
        cmd.addDouble(ball_pos[0]);
        cmd.addDouble(ball_pos[1]);
        cmd.addDouble(ball_pos[2]);
        cmd.addDouble(ball_col[0]);
        cmd.addDouble(ball_col[1]);
        cmd.addDouble(ball_col[2]);
        RTF_ASSERT_ERROR_IF(ballPort.write(cmd,reply), "Unable to talk to world");
    }

    /****************************************************/
    void deleteBall()
    {
        Bottle cmd,reply;
        cmd.addString("world");
        cmd.addString("del");
        cmd.addString("all");

        RTF_ASSERT_ERROR_IF(ballPort.write(cmd,reply), "Unable to talk to world");
    }

public:
    /*****************************************************/
    TestAssignmentYarpFindRgb() :
        yarp::rtf::TestCase("TestAssignmentYarpFindRgb")
    {
    }

    /*****************************************************/
    virtual ~TestAssignmentYarpFindRgb()
    {
    }

    /*****************************************************/
    bool setup(yarp::os::Property& property) override
    {
        float rpcTmo=(float)property.check("rpc-timeout",Value(10.0)).asDouble();

        Property option;
        option.put("device","remote_controlboard");
        option.put("remote","/icubSim/head");
        option.put("local","/test-controller");

        RTF_ASSERT_ERROR_IF(driver.open(option),"Unable to connect to icubSim");
        driver.view(ienc);

        anglePort.open("/test/angle-port");
        RTF_ASSERT_ERROR_IF(Network::connect(anglePort.getName(),
                                             "/head/ang:i"),
                                "Unable to connect to left target");

        colorPort.open("/test/color-port");
        RTF_ASSERT_ERROR_IF(Network::connect("/head/color:o",colorPort.getName()),
                            "Unable to connect to right target");

        ballPort.open("/test/ball");
        RTF_TEST_REPORT(Asserter::format("Set rpc timeout = %g [s]",rpcTmo));
        ballPort.asPort().setTimeout(rpcTmo);
        RTF_ASSERT_ERROR_IF(Network::connect(ballPort.getName(),"/icubSim/world"),
                            "Unable to connect to /icubSim/world");

        Rand::init();

        RTF_TEST_REPORT("Correct setup!");

        return true;
    }

    /****************************************************/
    void tearDown() override
    {
        anglePort.close();
        colorPort.close();
        ballPort.close();
        driver.close();
    }

    /****************************************************/
    void run() override
    {
        RTF_TEST_REPORT("Running tests...");
        unsigned int score=0;

        // pick up a rgb color randomly
        // making sure that r!=g!=b
        Vector min(3),max(3);
        min[0]=0.0;   max[0]=0.33;
        min[1]=0.331; max[1]=0.66;
        min[2]=0.661; max[2]=1.0;
        ball_col=Rand::vector(min,max);

        // shuffle rgb randomly
        if (Rand::scalar()<0.5)
            swap(ball_col[0],ball_col[1]);
        if (Rand::scalar()<0.5)
            swap(ball_col[0],ball_col[2]);
        if (Rand::scalar()<0.5)
            swap(ball_col[1],ball_col[2]);

        ball_pos.resize(3,0.0);
        ball_pos[0]=-0.4;
        ball_pos[1]=0.925;
        ball_pos[2]=0.35;
        createBall();

        RTF_TEST_REPORT("Ball has been created!");

        Time::delay(5.0);

        RTF_TEST_REPORT("Sending the right angle value to the head controller...");
        Bottle &angle_bottle=anglePort.prepare();
        angle_bottle.clear();
        angle_bottle.addString("angle");
        angle_bottle.addDouble(-50.0);
        anglePort.write();

        Time::delay(5.0);
        RTF_TEST_REPORT("Testing effectiveness of the controller...");
        int nAxes; ienc->getAxes(&nAxes);
        vector<double> encs(nAxes);
        ienc->getEncoders(encs.data());
        if (abs(encs[2]) > 5.0)
           score+= 5;

        RTF_TEST_CHECK(score>0,Asserter::format(" ***** Partial score = %d/15 ***** ", score));

        Time::delay(5.0);

        RTF_TEST_REPORT("Testing ball color...");

        double r,g,b;
        Bottle *color_ball=colorPort.read();
        if (color_ball->size()==3)
        {
            r=color_ball->get(0).asDouble();
            g=color_ball->get(1).asDouble();
            b=color_ball->get(2).asDouble();

            r/=255.0;
            g/=255.0;
            b/=255.0;

            score+=5;
        }

        RTF_TEST_CHECK(score>0,Asserter::format(" ***** Partial score = %d/15 ***** ", score));

        checkColor(r,g,b, ball_col, score);

        RTF_TEST_CHECK(score>0,Asserter::format(" ***** Total score = %d/15 ***** ",score));
    }

    /****************************************************/
    void checkColor(const double &r, const double &g, const double &b,  Vector color_ball, unsigned int &score)
    {
        Vector hsv_true, hsv_received;
        hsv_true.resize(3,0.0);
        hsv_received.resize(3,0.0);

        hsv_received=computeHSV(r,g,b);

        hsv_true=computeHSV(color_ball[0], color_ball[1], color_ball[2]);

        if ((abs(hsv_true[0]-hsv_received[0])<5.0) && (abs(hsv_true[1]-hsv_received[1])<5.0) && (abs(hsv_true[2]-hsv_received[2])<35.0))
            score+=5;
    }

    /****************************************************/
    Vector computeHSV(double  r1, double g1, double b1)
    {
        Vector hsv;
        hsv.resize(3,0.0);

        double cmax, cmin;
        cmax=max(b1,max(r1,g1));
        cmin=min(r1,min(g1,b1));

        double delta=cmax-cmin;

        if (delta==0)
            hsv[0]=0.0;
        else if (cmax == r1)
            hsv[0]=(int)(60*(g1-b1)/delta) % 6;
        else if (cmax == g1)
            hsv[0]=60*((b1-r1)/delta+2);
        else if (cmax == b1)
            hsv[0]=60*((r1-g1)/delta+4);

        if (cmax ==0)
            hsv[1]=0;
        else
            hsv[1]=delta/cmax*100;

        hsv[2]=cmax*100;

        return hsv;
    }
};

PREPARE_PLUGIN(TestAssignmentYarpFindRgb)
