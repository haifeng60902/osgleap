/*
* Example leappointer
* Copyright (C) 2013 Johannes Scholz/vtxtech. All rights reserved.
*
* This file is licensed under the GNU Lesser General Public License 3 (LGPLv3),
* but distributed WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
*/

#include <osg/io_utils>
#include <osg/Material>
#include <osg/PositionAttitudeTransform>
#include <osg/ShapeDrawable>
#include <osg/ValueObject>
#include <osgDB/ReadFile>
#include <osgText/Text>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgLeap/Controller>
#include <osgLeap/HandState>
#include <osgLeap/HUDCamera>
#include <osgLeap/IntersectionController>
#include <osgLeap/IntersectionDevice>
#include <osgLeap/IntersectionUpdateCallback>

osg::ref_ptr<osg::Node> createText()
{
    osg::Geode* geode = new osg::Geode();

    std::string timesFont("fonts/arial.ttf");

    // turn lighting off for the text and disable depth test to ensure it's always ontop.
    osg::StateSet* stateset = geode->getOrCreateStateSet();
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::Vec3 position(150.0f,200.0f,0.0f);
    osg::Vec3 delta(0.0f,-120.0f,0.0f);

    {
        osgText::Text* text = new  osgText::Text;
        geode->addDrawable( text );

        text->setFont(timesFont);
        text->setPosition(position);
        text->setText("Start your Leap Motion pointing at the screen!");

        position += delta;
    }

    return geode;
}

int main(int argc, char** argv)
{
    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc,argv);

    arguments.getApplicationUsage()->setApplicationName(arguments.getApplicationName());
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName()+" is an example showing osgLeap::IntersectionController use.");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName()+" [options] filename ...");
    arguments.getApplicationUsage()->addCommandLineOption("--timebased", "Invoke mouse clicks after some time if the pointer is not moving (Default)");
    arguments.getApplicationUsage()->addCommandLineOption("--noclick", "Initialize osgLeap::IntersectionDevice without ability to send clicks");
    arguments.getApplicationUsage()->addCommandLineOption("--screentap", "Invoke mouse clicks upon the screen tap gesture");

    osgViewer::Viewer viewer(arguments);

    unsigned int helpType = 0;
    if ((helpType = arguments.readHelpType()))
    {
        arguments.getApplicationUsage()->write(std::cout, helpType);
        return 1;
    }

    // report any errors if they have occurred when parsing the program arguments.
    if (arguments.errors())
    {
        arguments.writeErrorMessages(std::cout);
        return 1;
    }

    if (arguments.argc()<=1)
    {
        arguments.getApplicationUsage()->write(std::cout,osg::ApplicationUsage::COMMAND_LINE_OPTION);
        return 1;
    }

    viewer.addEventHandler(new osgViewer::WindowSizeHandler);

    // ToDo: Remove this...
    viewer.setThreadingModel(osgViewer::ViewerBase::SingleThreaded);

    // Defines the time that a pointer needs to stand still
    // before a mouse click is performed at the current position
    int clickEmulateStillStandTime = 3000;

    osgLeap::IntersectionDevice::ClickMode mode = osgLeap::IntersectionDevice::TIMEBASED_MOUSECLICK;
    while (arguments.read("--noclick")) {
        mode = osgLeap::IntersectionDevice::NONE;
        clickEmulateStillStandTime = 0;
    }
    while (arguments.read("--timebased")) {
    	mode = osgLeap::IntersectionDevice::TIMEBASED_MOUSECLICK;
    }
    while (arguments.read("--screentap")) {
        mode = osgLeap::IntersectionDevice::SCREENTAP;
        osgLeap::Controller::instance()->enableGesture(Leap::Gesture::TYPE_SCREEN_TAP);
        clickEmulateStillStandTime = 0;
    }

    // load the data
    osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFiles(arguments);
    if (!loadedModel)
    {
        std::cout << arguments.getApplicationName() <<": No data loaded" << std::endl;
        return 1;
    }

    // any option left unread are converted into errors to write out later.
    arguments.reportRemainingOptionsAsUnrecognized();

    // report any errors if they have occurred when parsing the program arguments.
    if (arguments.errors())
    {
        arguments.writeErrorMessages(std::cout);
        return 1;
    }

    viewer.setSceneData( loadedModel.get() );

    viewer.realize();

    // set up cameras to render on the first window available.
    osgViewer::Viewer::Windows windows;
    viewer.getWindows(windows);

    if (windows.empty()) return 1;

    osg::Camera* hudCamera = new osgLeap::HUDCamera(windows[0]);

    // Adds the osgLeap::HandState visualizer
    hudCamera->addChild(new osgLeap::HandState());

    // Add some text to the HUD
    hudCamera->addChild(createText());

    viewer.addSlave(hudCamera, false);

    osg::ref_ptr<osg::Group> pointersGroup = new osg::Group();
    // IntersectionUpdateCallback needs clickEmulateStillStandTime to visualize
    // the remaining time until the click is executed.
    osg::ref_ptr<osgLeap::IntersectionUpdateCallback> puc = new osgLeap::IntersectionUpdateCallback(hudCamera, clickEmulateStillStandTime);
    pointersGroup->addUpdateCallback(puc);
    hudCamera->addChild(pointersGroup);

    // Our IntersectionDevice is initialized to fire mouseclicks after clickEmulateStillStandTime is gone
    viewer.addDevice(new osgLeap::IntersectionDevice(mode, clickEmulateStillStandTime, puc->getIntersectionController()));

    viewer.addSlave(hudCamera, false);

    return viewer.run();

}
