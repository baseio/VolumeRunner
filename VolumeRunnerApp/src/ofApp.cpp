#include "ofApp.h"
#include "colormotor.h"
#include "AnimSys.h"
#include "RunningSkeleton.h"

#include "threaded_player.h"
#include "maximilian.h"
//ofBoxPrimitive box;

ofImage shapeImage;
MaxiThread maxi;



//This shows how to use maximilian to build a polyphonic synth.

//These are the synthesiser bits
maxiOsc VCO1[6],VCO2[6],LFO1[6],LFO2[6];
maxiFilter VCF[6];
maxiEnvelope ADSR[6];

//These are the control values for the envelope

double adsrEnv[8]={1,5,0.125,100,0.125,200,0,1000};

//This is a bunch of control signals so that we can hear something

maxiOsc timer;//this is the metronome
int currentCount,lastCount,voice=0;//these values are used to check if we have a new beat this sample

//and these are some variables we can use to pass stuff around

double VCO1out[6],VCO2out[6],LFO1out[6],LFO2out[6],VCFout[6],ADSRout[6],mix,pitch[6];


void setup() {//some inits
    
    
}

void play(double *output) {
    
    mix=0;//we're adding up the samples each update and it makes sense to clear them each time first.
    
    //so this first bit is just a basic metronome so we can hear what we're doing.
    
    currentCount=(int)timer.phasor(8);//this sets up a metronome that ticks 8 times a second
    
    if (lastCount!=currentCount) {//if we have a new timer int this sample, play the sound
        
        if (voice==6) {
            voice=0;
        }
        
        ADSR[voice].trigger(0, adsrEnv[0]);//trigger the envelope from the start
        pitch[voice]=voice+1;
        voice++;
        
        lastCount=0;
        
    }
    
    //and this is where we build the synth
    
    for (int i=0; i<6; i++) {
        
        
        ADSRout[i]=ADSR[i].line(8,adsrEnv);//our ADSR env has 8 value/time pairs.
        
        LFO1out[i]=LFO1[i].sinebuf(0.2);//this lfo is a sinewave at 0.2 hz
        
        VCO1out[i]=VCO1[i].pulse(55*pitch[i],0.6);//here's VCO1. it's a pulse wave at 55 hz, with a pulse width of 0.6
        VCO2out[i]=VCO2[i].pulse((110*pitch[i])+LFO1out[i],0.2);//here's VCO2. it's a pulse wave at 110hz with LFO modulation on the frequency, and width of 0.2
        
        
        VCFout[i]=VCF[i].lores((VCO1out[i]+VCO2out[i])*0.5, 250+((pitch[i]+LFO1out[i])*1000), 10);//now we stick the VCO's into the VCF, using the ADSR as the filter cutoff
        
        mix+=VCFout[i]*ADSRout[i]/6;//finally we add the ADSR as an amplitude modulator 
        
        
    }
    
    output[0]=mix*0.5;//left channel
    output[1]=mix*0.5;//right channel
    
}




ofSpherePrimitive sphere(1, 12);
ofVec3f floorPos;


//--------------------------------------------------------------
void ofApp::setup(){
    maxi.startThread();
    
    ofDisableArbTex();
    
    ofBackground(0, 0, 0);
    ofSetLogLevel(OF_LOG_VERBOSE);
    
    // initialize the dude before hand because of the parameters in the walking animation
    dude.init();
    volume.init();
    floor.init();
    
    params.addFloat("FPS").setRange(0, 60).setClamp(false);
    params.startGroup("Update"); {
        params.addBool("Pause").set(false);
        params.addBool("Dude").set(true);
    } params.endGroup();
    
    params.startGroup("Display"); {
        params.addBool("Debug skeleton").set(true);
        params.addBool("Volume").set(true);
        params.addBool("Sea").set(true);
        params.addBool("fbo").trackVariable(&renderManager.bUseFbo);
        params.addFloat("fbo size").setRange(0, 1).setSnap(true).set(0.5);
    } params.endGroup();
    
    
    params.startGroup("Shader"); {
        //        params.startGroup("Test box"); {
        //            params.addFloat("posx").setRange(-100, 100).setIncrement(1.0).setSnap(true);
        //            params.addFloat("posy").setRange(-100, 100).setIncrement(1.0).setSnap(true);
        //            params.addFloat("posz").setRange(-100, 100).setIncrement(1.0).setSnap(true);
        //            params.addFloat("rotx").setRange(-180, 180).setIncrement(1.0).setSnap(true);
        //            params.addFloat("roty").setRange(-180, 180).setIncrement(1.0).setSnap(true);
        //            params.addFloat("rotz").setRange(-180, 180).setIncrement(1.0).setSnap(true);
        //            params.addFloat("scalex").setRange(0, 50).setIncrement(0.1).setSnap(true);
        //            params.addFloat("scaley").setRange(0, 50).setIncrement(0.1).setSnap(true);
        //            params.addFloat("scalez").setRange(0, 50).setIncrement(0.1).setSnap(true);
        //        } params.endGroup();
        
        
        params.startGroup("View"); {
            params.addFloat("distance").setRange(0, 100).setIncrement(1.0);
            params.addFloat("rotx").setRange(-180, 10).setIncrement(1.0);
            params.addFloat("roty").setRange(-720, 720).setIncrement(1.0);
        } params.endGroup();
        
    } params.endGroup();
    
    dude.addParams(params);
    volume.addParams(params);
    floor.addParams(params);
    
    params.loadXmlValues();
    
    gui.addPage(params);
    gui.setDefaultKeys(true);
    gui.show();
    
    shaderFolderWatcher = new cm::FileWatcher(ofToDataPath("shaders"),500);
    shaderFolderWatcher->startThread();
    
    loadShaders();
    
    ofSetWindowShape(ofGetScreenWidth() * 0.5, ofGetScreenWidth() * 0.5);
    ofSetWindowPosition(0, 0);
    
    
    shapeImage.load("images/test.png");
    shapeImage.getTexture().setTextureWrap(GL_REPEAT,GL_REPEAT);
    //shapeImage.getTexture().setTextureMinMagFilter(GL_NEAREST,GL_NEAREST);
    //    cam = new ofCamera();
}

//--------------------------------------------------------------
void ofApp::loadShaders() {
    ofLogVerbose() << "*** Loading Shaders ***";
    
    shaderRayTracer = shared_ptr<ofShader>(new ofShader());
    shaderRayTracer->load("", "shaders/dani_testbed.frag");
    
    shaderSea = shared_ptr<ofShader>(new ofShader());
    shaderSea->load("", "shaders/sea.frag");
}

void ofApp::allocateFbo() {
    ofLogVerbose() << "*** allocateFbo ***";
    renderManager.allocate(ofGetWidth() * (float)params["Display.fbo size"], ofGetHeight() * (float)params["Display.fbo size"]);
}

//--------------------------------------------------------------
void ofApp::update(){
    if(params["Update.Pause"]) return;
    
    dude.updateParams(params);
    

    if(params["Update.Dude"]) {
//        Vec3 dudeLowest = dude.getLowestLimbPosition();
        floorPos.set(dude.position.x, floor.getHeight(dude.position.x, dude.position.z,shapeImage), dude.position.z);
        dude.floorHeight = floorPos.y;
        dude.update();
        
    }
    
    
    camera.rotx = params["Shader.View.rotx"];
    camera.roty = params["Shader.View.roty"];
    camera.distance = params["Shader.View.distance"];
    //dude.position(0,0,0);
    camera.update(dude.position, renderManager.getWidth(), renderManager.getHeight(), 0.1);//Vec3(0,0,0));
}

//--------------------------------------------------------------
void ofApp::draw(){
    params["FPS"] = ofGetFrameRate();
    
    if( shaderFolderWatcher->hasFileChanged() ) {
        loadShaders();
    }
    
    if(params["Display.fbo size"].hasChanged()) allocateFbo();
    
    if(!params["Update.Pause"]) {
        renderManager.begin();
        
        gfx::enableDepthBuffer(false);
        gfx::setIdentityTransform();
        
        
        if(params["Display.Sea"]) {
            shaderSea->begin();
            shaderSea->setUniform2i("iResolution", renderManager.getWidth(), renderManager.getHeight());
            shaderSea->setUniform2i("iMouse", ofGetMouseX(), ofGetMouseY());
            shaderSea->setUniform1i("iGlobalTime", ofGetElapsedTimeMillis());
            drawUVQuad();
            shaderSea->end();
        }
        
        shaderRayTracer->begin();
        shaderRayTracer->setUniform2f("resolution", renderManager.getWidth(), renderManager.getHeight());
        shaderRayTracer->setUniform1f("time", ofGetElapsedTimef());

        //    shaderRayTracer->setUniform3f("box_pos", params["Shader.Test box.posx"], params["Shader.Test box.posy"], params["Shader.Test box.posz"]);
        //    shaderRayTracer->setUniform3f("box_rot", ofDegToRad(params["Shader.Test box.rotx"]), ofDegToRad(params["Shader.Test box.roty"]), ofDegToRad(params["Shader.Test box.rotz"]));
        //    shaderRayTracer->setUniform3f("box_scale", params["Shader.Test box.scalex"], params["Shader.Test box.scaley"], params["Shader.Test box.scalez"]);
        
        dude.updateRenderer(*shaderRayTracer);
        camera.updateRenderer(*shaderRayTracer);
        floor.updateRenderer(*shaderRayTracer, shapeImage);
        
        drawUVQuad();
        shapeImage.unbind();
        shaderRayTracer->end();
        
        camera.apply();
        
        ofPushMatrix();
        ofPushStyle();
        ofTranslate(floorPos);
        ofSetColor(255, 0, 0);
        sphere.draw();
        ofPopStyle();
        ofPopMatrix();
        
        if(params["Display.Debug skeleton"]) {
            dude.debugDraw();
        }
        
        
        if(params["Display.Volume"]) {
            
            gfx::pushMatrix();
            M44 m;
            m.identity();
            m.translate(dude.position);
            m *= dude.bodyBone->matrix;
            // bone is oriented towards z, so rotate the head.
            m.rotateX(radians(90));
            m.rotateY(radians(dude.heading));
            gfx::applyMatrix(m);
            
            volume.draw(ofVec3f(0,0,0));//dude.position.x, dude.position.y, dude.position.z));
            gfx::popMatrix();
        }
        renderManager.end();
    }
    
    ofSetupScreen();
    renderManager.draw(0, ofGetHeight(), ofGetWidth(), -ofGetHeight());
    shapeImage.getTexture().bind();
    //drawUVQuad(0,0,512,512);
    shapeImage.getTexture().unbind();
    ofSetColor(255, 0, 0);
    ofDrawBitmapString(ofToString(ofGetFrameRate()), ofGetWidth() - 100, 20);
}

//--------------------------------------------------------------
void ofApp::exit(){
    //    params.saveXmlValues();
    shaderFolderWatcher->stopThread();
    delete shaderFolderWatcher;
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    float rotspeed = params["Dude.Rot speed"];
    switch(key) {
        case 's': params.saveXmlValues(); break;
        case 'l': params.loadXmlValues(); break;
        case 'f': ofToggleFullscreen(); break;
        case 'p': params["Update.Pause"] = ! params["Update.Pause"]; break;
        case 'r': dude.position(0, 0, 0); camera.target(0, 0, 0); break;
            
        case OF_KEY_LEFT:
            dude.heading += (rotspeed);
            break;    // steer left
            
        case OF_KEY_RIGHT:
            dude.heading -= (rotspeed);
            break;   // steer right
            
        case OF_KEY_UP:
            params["Dude.speed"] = (float)params["Dude.speed"] + 1.0;
            break;
            
        case OF_KEY_DOWN:
            params["Dude.speed"] =  (float)params["Dude.speed"] - 1.0;
            break;

        case '1':
            dude.playAnimation("run");
            break;

        case '2':
            dude.playAnimation("box");
            break;
        
        case '3':
            dude.playAnimation("skip");
            break;
            
            
            //case '1':
            
            
            //        case 'S': params.saveXmlSchema();
            //        case 'L': params.loadXmlSchema();
    }
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    float roty = (float)params["Shader.View.roty"] - (x - ofGetPreviousMouseX()) * ofGetLastFrameTime() * 10.0;
    if(roty<-180)
        roty+=360;
    if(roty>180)
        roty-=360;
    params["Shader.View.roty"] = roty;
    
    params["Shader.View.rotx"] = (float)params["Shader.View.rotx"] - (y - ofGetPreviousMouseY()) * ofGetLastFrameTime() * 10.0;
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    allocateFbo();
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 
    
}
