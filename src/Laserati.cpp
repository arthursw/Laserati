#include "Laserati.h"

void Laserati::setup(){
    ofSetVerticalSync(false);
    ofSetFrameRate(60);
    ofTrueTypeFont::setGlobalDpi(72);
	verdana14.loadFont("verdana.ttf", 14, true, true);
    
    ofSetCircleResolution(50);
    
	cameraWidth 		= 640;
	cameraHeight 		= 480;
    
	camera.setDeviceID(0);
	camera.setDesiredFrameRate(60);
	camera.initGrabber(cameraWidth,cameraHeight);

    threshold = 240;
    
    spaceShipImg.loadImage("images/spaceShip.png");
    spaceShipPos = ofPoint(0,0);
    
    calibrationStep = 0;
    calibrationStepCheck = false;
    showCamera = true;
    selectedCorner = -1;
    dragging = false;
    
    // setup a server with default options on port 9092
    // - pass in true after port to set up with SSL
    //bConnected = server.setup( 9093 );
    
    ofxLibwebsockets::ServerOptions options = ofxLibwebsockets::defaultServerOptions();
    options.port = 9092;
    bConnected = server.setup( options );
    
    
    // this adds your app as a listener for the server
    server.addListener(this);
    
}

ofVec3f Laserati::multiplyMatrixVector3(ofMatrix3x3 &m, ofVec3f &v)
{
    ofVec3f r;
    
    /**
     * [ a b c ]
     * [ d e f ]
     * [ g h i ]
     */
    
    r.x = m.a*v.x + m.b*v.y + m.c*v.z;
    r.y = m.d*v.x + m.e*v.y + m.f*v.z;
    r.z = m.g*v.x + m.h*v.y + m.i*v.z;
    
    return r;
}

void Laserati::update(){
    camera.update();
    
	if (camera.isFrameNew())
    {
		unsigned char * pixels = camera.getPixels();
        
        int nPoints = 0;
        
        laserPos.x = -1.0;
        laserPos.y = -1.0;
        
        for(int i=0 ; i<camera.height ; i+=2)
        {
            for(int j=0 ; j<camera.width ; j+=2)
            {
                int idx = 3*(i*cameraWidth+j);
                
                if(pixels[idx+0]>threshold && pixels[idx+1]>threshold && pixels[idx+2]>threshold)
                {
                    if(laserPos.x == -1.0)
                    {
                        laserPos.x = j;
                        laserPos.y = i;
                    }
                    nPoints++;
                    laserPos.x = ((nPoints-1)*laserPos.x+j)/double(nPoints);
                    laserPos.y = ((nPoints-1)*laserPos.y+i)/double(nPoints);
                }
            }
        }
        
        if(corners.size()==4)
        {
            
            
            if(calibrationStep==4)
            {
                ofMatrix3x3 srcMat;
                
                /**
                 * [ a b c ]
                 * [ d e f ]
                 * [ g h i ]
                 */
                
                srcMat.a = corners[0].x;
                srcMat.b = corners[1].x;
                srcMat.c = corners[2].x;
                
                srcMat.d = corners[0].y;
                srcMat.e = corners[1].y;
                srcMat.f = corners[2].y;
                
                srcMat.g = 1.0;
                srcMat.h = 1.0;
                srcMat.i = 1.0;
                
                ofMatrix3x3 srcMatInv = srcMat;
                srcMatInv.invert();
                
                ofVec3f c4(corners[3].x, corners[3].y, 1.0);
                ofVec3f srcVars = multiplyMatrixVector3(srcMatInv, c4);
                
                ofMatrix3x3 A(corners[0].x*srcVars.x, corners[1].x*srcVars.y, corners[2].x*srcVars.z, corners[0].y*srcVars.x, corners[1].y*srcVars.y, corners[2].y*srcVars.z, srcVars.x, srcVars.y, srcVars.z);
                
                int w = ofGetWidth();
                int h = ofGetHeight();
                ofMatrix3x3 dstMat(0, w, w, 0, 0, h, 1, 1, 1);
                
                ofMatrix3x3 dstMatInv = dstMat;
                dstMatInv.invert();
                
                ofVec3f d4(0.0, h, 1.0);
                ofVec3f dstVars = multiplyMatrixVector3(dstMatInv, d4);
                
                ofMatrix3x3 B(0, w*dstVars.y, w*dstVars.z, 0, 0, h*dstVars.z, dstVars.x, dstVars.y, dstVars.z);
                
                ofMatrix3x3 Ainv = A;
                Ainv.invert();
                
//                C = B*Ainv;

                ofMatrix3x3 Binv = B;
                Binv.invert();
                
                C = A*Binv;
                
                C.invert();
                showCamera = false;
                calibrationStep++;
            }
            
            /*
             pos1 = @worldToScreen(@sprites[0].position)
             pos2 = @worldToScreen(@sprites[1].position)
             pos3 = @worldToScreen(@sprites[2].position)
             pos4 = @worldToScreen(@sprites[3].position)
             
             srcMat = new THREE.Matrix3(pos1.x, pos2.x, pos3.x, pos1.y, pos2.y, pos3.y, 1, 1, 1)
             srcMatInv = @inverseMatrix(srcMat)
             srcVars = @multiplyMatrixVector(srcMatInv, new THREE.Vector3(pos4.x, pos4.y, 1))
             A = new THREE.Matrix3(pos1.x*srcVars.x, pos2.x*srcVars.y, pos3.x*srcVars.z, pos1.y*srcVars.x, pos2.y*srcVars.y, pos3.y*srcVars.z, srcVars.x, srcVars.y, srcVars.z)
             
             dstMat = new THREE.Matrix3(0, 1, 0, 1, 1, 0, 1, 1, 1)
             dstMatInv = @inverseMatrix(dstMat)
             dstVars = @multiplyMatrixVector(dstMatInv, new THREE.Vector3(1, 0, 1))
             B = new THREE.Matrix3(0, dstVars.y, 0, dstVars.x, dstVars.y, 0, dstVars.x, dstVars.y, dstVars.z)
             
             Ainv =  @inverseMatrix(A)
             
             C = @multiplyMatrices(B,Ainv)
             
             ce = C.elements
             @uniforms.matC.value = new THREE.Matrix4(ce[0], ce[3], ce[6], 0, ce[1], ce[4], ce[7], 0, ce[2], ce[5], ce[8], 0, 0, 0, 0, 0)
             
             
             vec4 fragCoordH = vec4(gl_FragCoord.xy/resolution, 1, 0);
             vec4 uvw_t = matC*fragCoordH;
             vec2 uv_t = vec2(uvw_t.x/uvw_t.z, uvw_t.y/uvw_t.z);
             
             */
            
        }
    }
    spaceShipPos.x += 10;
    
    if(spaceShipPos.x>ofGetWidth())
    {
        spaceShipPos.x = -spaceShipImg.width;
        spaceShipPos.y += 10;
    }
}

void Laserati::draw()
{
    calibrationStepCheck = true;
    
    
    ofSetColor(255, 255, 255);
    
    if(showCamera)
        camera.draw(0,0);
    
//    if(calibrationStep==0)
//        qrCodeImg.draw(0,0);
//    else if (calibrationStep==1)
//        qrCodeImg.draw(ofGetWidth()-qrCodeImg.width,0);
//    else if (calibrationStep==2)
//        qrCodeImg.draw(ofGetWidth()-qrCodeImg.width,ofGetHeight()-qrCodeImg.height);
//    else if (calibrationStep==3)
//        qrCodeImg.draw(0,ofGetHeight()-qrCodeImg.height);
    
    ofSetColor(255, 75, 75);
    ofFill();
    
    for(int i=0 ; i<corners.size() ; i++)
    {
        if(i==selectedCorner)
            ofSetColor(75, 255, 75);
            
        ofCircle(corners[i], 3);
        ofSetColor(255, 75, 75);
    }
    
    ofSetColor(75, 75, 255);
    ofFill();
    
    for(int i=0 ; i<corners.size() ; i++)
    {
        ofVec3f corner3(corners[i].x, corners[i].y, 1.0);
        ofVec3f cornerPosOnScreen3 = multiplyMatrixVector3(C, corner3);
        ofVec2f cornerPosOnScreen(cornerPosOnScreen3.x/cornerPosOnScreen3.z, cornerPosOnScreen3.y/cornerPosOnScreen3.z);
        ofCircle(cornerPosOnScreen, 3);
    }

    ofSetColor(150, 250, 75);
    ofNoFill();
    ofCircle(laserPos, 5);
    
    ofSetColor(255, 255, 255);
    
    spaceShipImg.draw(spaceShipPos);
    
    ofVec3f laserPos3(laserPos.x, laserPos.y, 1.0);
    ofVec3f laserPosOnScreen3 = multiplyMatrixVector3(C, laserPos3);
    ofVec2f laserPosOnScreen(laserPosOnScreen3.x/laserPosOnScreen3.z, laserPosOnScreen3.y/laserPosOnScreen3.z);
    
    ofSetColor(150, 250, 75);
    ofFill();
    ofCircle(laserPosOnScreen, 5);

    ofSetColor(255, 255, 255);
    
    if(spaceShipPos.y>ofGetHeight()-spaceShipImg.height)
        verdana14.drawString("You loose", ofGetWidth()/2.0-50, ofGetHeight()/2.0-15);
    
    if(laserPosOnScreen.x > spaceShipPos.x && laserPosOnScreen.x < spaceShipPos.x+spaceShipImg.width && laserPosOnScreen.y > spaceShipPos.y && laserPosOnScreen.y < spaceShipPos.y+spaceShipImg.height)
        verdana14.drawString("You win", ofGetWidth()/2.0-40, ofGetHeight()/2.0-15);
    
    // debug
    
    std::ostringstream s;
    s << "Threshold: " << threshold;
    verdana14.drawString(s.str(), ofGetWidth()/2.0-40, ofGetHeight()-25);
    std::ostringstream s2;
    s2 << "Move step: " << cornerMoveStep;
    verdana14.drawString(s2.str(), ofGetWidth()/2.0-40, ofGetHeight()-15);
}

void Laserati::keyPressed(int key){
    ofVec2f &selectedC = corners[selectedCorner];

    if(key == OF_KEY_UP)
        selectedC.y-=1;
    else if(key == OF_KEY_DOWN)
        selectedC.y+=1;
    if(key == OF_KEY_LEFT)
        selectedC.x-=1;
    else if(key == OF_KEY_RIGHT)
        selectedC.x+=1;
    
    if(key==OF_KEY_UP || key==OF_KEY_DOWN || key==OF_KEY_LEFT || key==OF_KEY_RIGHT)
        calibrationStep = 4;
    
    if(key == 'p')
        threshold += 5;
    else if(key == 'm')
        threshold -= 5;
    
    if(key == 'o')
        cornerMoveStep += 0.5;
    else if(key == 'l')
        cornerMoveStep -= 0.5;
    
    else if(key == 'c')
        showCamera = !showCamera;
    else if(key == 's')
        camera.videoSettings();
    
    if ( key == ' ' ){
        string url = "http";
        if ( server.usingSSL() ){
            url += "s";
        }
        url += "://localhost:" + ofToString( server.getPort() );
        ofLaunchBrowser(url);
    }
}

void Laserati::keyReleased(int key){

}

void Laserati::mouseMoved(int x, int y){

}

void Laserati::mouseDragged(int x, int y, int button){
    if(dragging)
    {
        corners[selectedCorner].x = x;
        corners[selectedCorner].y = y;
    }
}

void Laserati::mousePressed(int x, int y, int button){
    if(corners.size()<4)
    {
        cout << "calibrationStep: " << calibrationStep << endl;
        
        corners.push_back(ofVec2f(x, y));
        calibrationStep++;
        
        calibrationStepCheck = false;
    }
    
    cout << "--- x: " << x << ", y: " << y << endl;
    
    selectedCorner = -1;
    for(int i=0 ; i<corners.size() ; i++)
    {
        cout << "x: " << corners[i].x << ", y: " << corners[i].y << endl;
        if(x>corners[i].x-5 && x<corners[i].x+5 && y>corners[i].y-5 && y<corners[i].y+5)
        {
            selectedCorner = i;
            dragging = true;
        }
    }
    
    cout << "--- selectedCorner: " << selectedCorner << endl;
}

void Laserati::mouseReleased(int x, int y, int button){
    if(dragging)
        calibrationStep = 4;
    dragging = false;
}

void Laserati::windowResized(int w, int h){

}

void Laserati::gotMessage(ofMessage msg){

}

void Laserati::dragEvent(ofDragInfo dragInfo){ 

}


//--------------------------------------------------------------
//----------------- Lib Websockets -----------------------------
//--------------------------------------------------------------
void Laserati::onConnect( ofxLibwebsockets::Event& args ){
    cout<<"on connected"<<endl;
}

//--------------------------------------------------------------
void Laserati::onOpen( ofxLibwebsockets::Event& args ){
    cout<<"new connection open"<<endl;
    cout<<args.conn.getClientIP()<< endl;
//    
//    Drawing * d = new Drawing();
//    d->_id = canvasID++;
//    d->color.set(ofRandom(255),ofRandom(255),ofRandom(255));;
//    d->conn = &( args.conn );
//    
//    drawings.insert( make_pair( d->_id, d ));
//    
//    // send "setup"
//    args.conn.send( d->getJSONString("setup") );
//    
//    // send drawing so far
//    map<int, Drawing*>::iterator it = drawings.begin();
//    for (it; it != drawings.end(); ++it){
//        Drawing * drawing = it->second;
//        if ( d != drawing ){
//            for ( int i=0; i<drawing->points.size(); i++){
//                string x = ofToString(drawing->points[i].x);
//                string y = ofToString(drawing->points[i].y);
//                server.send( "{\"id\":"+ ofToString(drawing->_id) + ",\"point\":{\"x\":\""+ x+"\",\"y\":\""+y+"\"}," + drawing->getColorJSON() +"}");
//            }
//        }
//    }
}

//--------------------------------------------------------------
void Laserati::onClose( ofxLibwebsockets::Event& args ){
    cout<<"on close"<<endl;
}

//--------------------------------------------------------------
void Laserati::onIdle( ofxLibwebsockets::Event& args ){
    cout<<"on idle"<<endl;
}

//--------------------------------------------------------------
void Laserati::onMessage( ofxLibwebsockets::Event& args ){
    cout<<"got message "<<args.message<<endl;
    
    try{
//        // trace out string messages or JSON messages!
//        if ( !args.json.isNull() ){
//            ofPoint point = ofPoint( args.json["point"]["x"].asFloat(), args.json["point"]["y"].asFloat() );
//            
//            // for some reason these come across as strings via JSON.stringify!
//            int r = ofToInt(args.json["color"]["r"].asString());
//            int g = ofToInt(args.json["color"]["g"].asString());
//            int b = ofToInt(args.json["color"]["b"].asString());
//            ofColor color = ofColor( r, g, b );
//            
//            int _id = ofToInt(args.json["id"].asString());
//            
//            map<int, Drawing*>::const_iterator it = drawings.find(_id);
//            Drawing * d = it->second;
//            d->addPoint(point);
//        } else {
//        }
//        // send all that drawing back to everybody except this one
//        vector<ofxLibwebsockets::Connection *> connections = server.getConnections();
//        for ( int i=0; i<connections.size(); i++){
//            if ( (*connections[i]) != args.conn ){
//                connections[i]->send( args.message );
//            }
//        }
    }
    catch(exception& e){
        ofLogError() << e.what();
    }
}

//--------------------------------------------------------------
void Laserati::onBroadcast( ofxLibwebsockets::Event& args ){
    cout<<"got broadcast "<<args.message<<endl;    
}
