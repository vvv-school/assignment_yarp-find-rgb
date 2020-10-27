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

#include <robottestingframework/dll/Plugin.h>
#include <robottestingframework/TestAssert.h>

#include <yarp/robottestingframework/TestCase.h>
#include <yarp/os/all.h>
#include <yarp/dev/all.h>
#include <yarp/sig/all.h>
#include <yarp/math/Math.h>
#include <yarp/math/Rand.h>

#include <iCub/ctrl/math.h>

using namespace std;
using namespace robottestingframework;
using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::sig;
using namespace yarp::math;
using namespace iCub::ctrl;

class TestAssignmentYarpFindRgb : public yarp::robottestingframework::TestCase
{
    PolyDriver                       driver;
    IEncoders                       *ienc;

    BufferedPort<Bottle>             anglePort;
    BufferedPort<Bottle>             colorPort;
    RpcClient                        ballPort;

    Vector                           ball_col;
    Vector                           ball_pos;

    std::string                      m_ball_name;

    /****************************************************/
    void createBall()
    {
        Bottle cmd, reply;
        cmd.addString("makeSphere");
        cmd.addFloat64(0.06); // radius
        cmd.addFloat64(ball_pos[0]); // pose x
        cmd.addFloat64(ball_pos[1]); // pose y
        cmd.addFloat64(ball_pos[2]); // pose z
        cmd.addFloat64(ball_pos[3]); // pose theta_0
        cmd.addFloat64(ball_pos[4]); // pose theta_1
        cmd.addFloat64(ball_pos[5]); // pose theta_2
        cmd.addInt16(static_cast<int16_t>(ball_col[0])); // R
        cmd.addInt16(static_cast<int16_t>(ball_col[1])); // G
        cmd.addInt16(static_cast<int16_t>(ball_col[2])); // B
        auto ok = ballPort.write(cmd, reply);

        if (ok && reply.size() > 0) {
            m_ball_name = reply.get(0).asString();
        }

        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(ok, "Unable to talk to world");
    }

    /****************************************************/
    void deleteBall()
    {
        Bottle cmd, reply;
        cmd.addString("deleteObject");
        cmd.addString(m_ball_name);

        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(ballPort.write(cmd,reply), "Unable to talk to world");
    }

public:
    /*****************************************************/
    TestAssignmentYarpFindRgb() :
        yarp::robottestingframework::TestCase("TestAssignmentYarpFindRgb")
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

        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(driver.open(option),"Unable to connect to icubSim");
        driver.view(ienc);

        anglePort.open("/test/angle-port");
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(Network::connect(anglePort.getName(),
                                                   "/head/ang:i"),
                                  "Unable to connect to left target");

        colorPort.open("/test/color-port");
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(Network::connect("/head/color:o",colorPort.getName()),
                                  "Unable to connect to right target");

        ballPort.open("/test/ball");
        ROBOTTESTINGFRAMEWORK_TEST_REPORT(Asserter::format("Set rpc timeout = %g [s]",rpcTmo));
        ballPort.asPort().setTimeout(rpcTmo);
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(Network::connect(ballPort.getName(),"/world_input_port/yarp-find-rgb"),
                                  "Unable to connect to /world_input_port/yarp-find-rgb");

        Rand::init();

        ROBOTTESTINGFRAMEWORK_TEST_REPORT("Correct setup!");

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
        ROBOTTESTINGFRAMEWORK_TEST_REPORT("Running tests...");
        unsigned int score=0;

        // pick up a rgb color randomly
        // making sure that r!=g!=b
        Vector min(3),max(3);
        min[0]=0.0;   max[0]=0.33;
        min[1]=0.331; max[1]=0.66;
        min[2]=0.661; max[2]=1.0;
        ball_col=255*Rand::vector(min,max);

        // shuffle rgb randomly
        if (Rand::scalar()<0.5)
            swap(ball_col[0],ball_col[1]);
        if (Rand::scalar()<0.5)
            swap(ball_col[0],ball_col[2]);
        if (Rand::scalar()<0.5)
            swap(ball_col[1],ball_col[2]);

        ball_pos.resize(6, 0.0);
        ball_pos[0] = -0.4;
        ball_pos[1] = 0.325;
        ball_pos[2] = 0.975;
        createBall();

        ROBOTTESTINGFRAMEWORK_TEST_REPORT("Ball has been created!");

        Time::delay(5.0);

        ROBOTTESTINGFRAMEWORK_TEST_REPORT("Sending the right angle value to the head controller...");
        Bottle &angle_bottle=anglePort.prepare();
        angle_bottle.clear();
        angle_bottle.addString("angle");
        angle_bottle.addDouble(-50.0);
        anglePort.write();

        Time::delay(8.0);
        ROBOTTESTINGFRAMEWORK_TEST_REPORT("Testing effectiveness of the controller...");
        int nAxes; ienc->getAxes(&nAxes);
        vector<double> encs(nAxes);
        ienc->getEncoders(encs.data());
        if (abs(encs[2]) > 5.0)
           score+= 5;

        ROBOTTESTINGFRAMEWORK_TEST_CHECK(score>0,Asserter::format(" ***** Partial score = %d/15 ***** ", score));

        Time::delay(5.0);

        ROBOTTESTINGFRAMEWORK_TEST_REPORT("Testing ball color...");

        double r,g,b;
        unsigned counter = 0;
        constexpr unsigned maxIter = 100;

        Bottle* color_ball = colorPort.read(false);

        while (!color_ball) {
            if (++counter == maxIter) {
                ROBOTTESTINGFRAMEWORK_TEST_REPORT("Failed to read from the color port...");
                ROBOTTESTINGFRAMEWORK_TEST_CHECK(score > 0, Asserter::format("Total score = %d", score));
                return;
            }
            // Sleep for some while
            yarp::os::Time::delay(0.05);
            color_ball = colorPort.read(false);
        }
        if (color_ball->size()==3)
        {
            r=color_ball->get(0).asDouble();
            g=color_ball->get(1).asDouble();
            b=color_ball->get(2).asDouble();

            score+=5;
        }

        ROBOTTESTINGFRAMEWORK_TEST_CHECK(score>0,Asserter::format(" ***** Partial score = %d/15 ***** ", score));

        checkColor(r,g,b, ball_col, score);

        ROBOTTESTINGFRAMEWORK_TEST_CHECK(score>0,Asserter::format("Total score = %d",score));
    }

    /****************************************************/
    void checkColor(const double &r, const double &g, const double &b,  Vector color_ball, unsigned int &score)
    {
        Vector hsv_true, hsv_received;
        hsv_true.resize(3,0.0);
        hsv_received.resize(3,0.0);

        hsv_received=computeHSV(r,g,b);

        hsv_true=computeHSV(color_ball[0], color_ball[1], color_ball[2]);

        yInfo("HSV true... %f %f %f", hsv_true[0], hsv_true[1], hsv_true[2]);
        yInfo("HSV received... %f %f %f", hsv_received[0], hsv_received[1], hsv_received[2]);

        // V is not considered in the check since is affect by the lighting
        if ((abs(hsv_true[0]-hsv_received[0])<5.0) && (abs(hsv_true[1]-hsv_received[1])<5.0))
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

ROBOTTESTINGFRAMEWORK_PREPARE_PLUGIN(TestAssignmentYarpFindRgb)
