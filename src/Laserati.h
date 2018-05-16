#pragma once

#include "ofMain.h"
#include "ofxLibwebsockets.h"

class Laserati : public ofBaseApp{
public:
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);

    ofVec3f multiplyMatrixVector3(ofMatrix3x3 &m, ofVec3f &v);
    
    ofImage spaceShipImg;
    ofVec2f spaceShipPos;
    
    ofVideoGrabber camera;
    bool showCamera;
    int cameraWidth;
    int cameraHeight;
    int threshold;
    
    ofVec2f laserPos;
    bool dragging;
    int selectedCorner;
    double cornerMoveStep;
    
    ofTrueTypeFont	verdana14;
    
    // calibration
    vector<ofVec2f> corners;
//    ofImage qrCodeImg;
    int calibrationStep;
    bool calibrationStepCheck;
    
    ofMatrix3x3 C;
    
    // lib websockets
    ofxLibwebsockets::Server server;
    bool bConnected;
    
    // websocket methods
    void onConnect( ofxLibwebsockets::Event& args );
    void onOpen( ofxLibwebsockets::Event& args );
    void onClose( ofxLibwebsockets::Event& args );
    void onIdle( ofxLibwebsockets::Event& args );
    void onMessage( ofxLibwebsockets::Event& args );
    void onBroadcast( ofxLibwebsockets::Event& args );
};
