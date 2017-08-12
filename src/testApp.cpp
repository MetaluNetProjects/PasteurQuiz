/*
 * Copyright (c) 2014 Antoine Rousseau <antoine@metalu.net>
 * BSD Simplified License, see the file "LICENSE.txt" in this distribution.
 * See https://github.com/Ant1r/ofxPof for documentation and updates.
 */

#include "testApp.h"
#include "pofBase.h"
#include "ofxAccelerometer.h"

using namespace std;
using namespace pd;

// externals setup declarations :
extern "C" {
    extern void limiter_tilde_setup();
    extern void z_tilde_setup(void);
    extern void seq_setup(void);
    extern void seq_setup(void);
    extern void midiparse_setup(void);
    extern void atan_tilde_setup();
    extern void urn_setup();
}

const char Tag[]="PasteurQuiz";
void setupJava();

//--------------------------------------------------------------
void testApp::setup() {

	// the number of libpd ticks per buffer,
	// used to compute the audio buffer len: tpb * blocksize (always 64)
	//#ifdef TARGET_LINUX_ARM
		// longer latency for Raspberry PI
		//int ticksPerBuffer = 32; // 32 * 64 = buffer len of 2048
		//int numInputs = 0; // no built in mic
	//#else
		int ticksPerBuffer = 1; // 8 * 64 = buffer len of 512
		int numInputs = 1;
		int sampleRate = 44100;
	//#endif
	
    ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofEnableSmoothing();

	ofLogNotice(Tag, "init sound");
	// setup OF sound stream
	//ofSoundStreamSetup(2, numInputs, this, 44100, ofxPd::blockSize()*ticksPerBuffer, 4);
	os = NULL;
	os = opensl_open(sampleRate, numInputs, 2, ticksPerBuffer*PdBase::blockSize(), testApp::opensl_process, (void*)this);
	if(os == NULL) ofLogError(Tag, "error opening opensl");

	ofxAccelerometer.setup();
	
	ofLogNotice(Tag, "init pd");
	if(!puda.init(2, numInputs, sampleRate, ticksPerBuffer)) {
		ofExit();
	}
		
	ofLogNotice(Tag, "init pof");
	pofBase::setup();
	
	ofLogNotice(Tag, "start pd");
	puda.start();
	
	puda.subscribe("toSYSTEM");
	// add message receiver, disables polling (see processEvents)
	puda.addReceiver(*this);   // automatically receives from all subscribed sources
	
	// ------------ load externals -----------------
	
	limiter_tilde_setup();
	z_tilde_setup();
    seq_setup();
    midiparse_setup();
    atan_tilde_setup();
    urn_setup();
    
    // ---------------------------------------------
	
	
	ofLogNotice(Tag, "load patch");
	Patch patch = puda.openPatch(ofToDataPath("pd/pof_main.pd"));
		
	if(os) opensl_start(os);
	
	// -----
	
	setupJava();
}

//--------------------------------------------------------------
void testApp::update() {
	pofBase::updateAll();
	while(dequeueToJava()>0);
}

//--------------------------------------------------------------
void testApp::draw() {
	pofBase::drawAll();
}

//--------------------------------------------------------------
//--------------------------------------------------------------
void testApp::exit() {
	ofLogNotice(Tag, "exit");
	if(os) opensl_close(os);
	pofBase::release();
	ofExit();
}

//--------------------------------------------------------------
void testApp::keyPressed(int key) {}

//--------------------------------------------------------------
void testApp::touchDown(int x, int y, int id){
	pofBase::touchDownAll(x, y, id);
}

void testApp::touchMoved(int x, int y, int id){
	pofBase::touchMovedAll(x, y, id);	
}

void testApp::touchUp(int x, int y, int id){
	pofBase::touchUpAll(x, y, id);
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h) {
	pofBase::windowResized(w,h);
}

//--------------------------------------------------------------
void testApp::audioReceived(float * input, int bufferSize, int nChannels) {
	puda.audioIn(input, bufferSize, nChannels);
}

//--------------------------------------------------------------
void testApp::audioRequested(float * output, int bufferSize, int nChannels) {
	puda.audioOut(output, bufferSize, nChannels);
}

//--------------------------------------------------------------
void testApp::reloadTextures() {
	pofBase::reloadTextures();
}

void testApp::unloadTextures() {
	pofBase::unloadTextures();
}

//--------------------------------------------------------------
short testInBuf[1024], testOutBuf[1024];
void testApp::opensl_process(void *app, int sample_rate, int buffer_frames,
			int input_channels, const short *input_buffer,
			int output_channels, short *output_buffer) {
	((testApp*)app)->puda.PdBase::processShort(/*buffer_frames*/1,(short *)input_buffer, output_buffer);
}

//--------------------------- JAVA -------------
jclass objClass = NULL;
jclass floatClass = NULL;
jclass intClass = NULL;
jclass stringClass = NULL;
jmethodID floatInit = NULL;

void setupJava(){
	JNIEnv *env = ofGetJNIEnv();
	
	if(objClass != NULL) return;
	
	objClass = (jclass)env->NewGlobalRef(env->FindClass("java/lang/Object"));
	stringClass = (jclass)env->NewGlobalRef(env->FindClass("java/lang/String"));
	floatClass = (jclass)env->NewGlobalRef(env->FindClass("java/lang/Float"));
	intClass = (jclass)env->NewGlobalRef(env->FindClass("java/lang/Integer"));
	floatInit = env->GetMethodID(floatClass, "<init>", "(F)V");
}
// forward messages from Pd to Java:
void testApp::receiveMessage(const std::string& dest, const std::string& msg, const List& list) {
	//ofLogNotice("OF") << "message " << dest << ": " << msg << " " << list.toString() << list.types() << endl;
	List *newlist = new List();
	*newlist << dest << msg;
	
	for(unsigned int i = 0; i < list.len(); ++i) {
		if(list.isFloat(i))
			*newlist << list.getFloat(i);
		else if(list.isSymbol(i))
			*newlist << list.getSymbol(i);
	}

	/*toJavaMutex.lock();
	toJavaQueue.push(*newlist);
	toJavaMutex.unlock();*/
	queueToJava(*newlist);
}

jobjectArray testApp::makeJavaArray(JNIEnv *env, const List& list) {
	jobjectArray jarray = env->NewObjectArray(list.len(), objClass, NULL);
  
	for(unsigned int i = 0; i < list.len(); ++i) {
    	jobject obj = NULL;
		if(list.isFloat(i))
			obj = env->NewObject(floatClass, floatInit, list.getFloat(i));
		else if(list.isSymbol(i))
			obj = env->NewStringUTF(list.getSymbol(i).c_str());

		env->SetObjectArrayElement(jarray, i, obj);
		if (obj != NULL) {
		  env->DeleteLocalRef(obj);  // The reference in the array remains.
		}
	}
	
	return jarray;
}

int testApp::dequeueToJava() {
	List *list = NULL;
	toJavaMutex.lock();
	int n = toJavaQueue.size();
	if(n > 0) {
		list = &toJavaQueue.front();
		toJavaMutex.unlock();
		sendToJava(*list);
		n--;
		toJavaMutex.lock();
		toJavaQueue.pop();
	}
	toJavaMutex.unlock();
	//if(list) sendToJava(*list);
	return n;
}

void testApp::sendToJava(const List& list) {
	JNIEnv *env = ofGetJNIEnv();
	jobject activity = ofGetOFActivityObject();
	jclass activityClass = env->FindClass("net/metalu/PasteurQuiz/OFActivity");
	jmethodID javaReceiveMessage = env->GetMethodID(activityClass,"receiveMessage","([Ljava/lang/Object;)V");

	jobjectArray jarray = makeJavaArray(env, list);
	env->CallVoidMethod(activity, javaReceiveMessage, jarray);
	env->DeleteLocalRef(jarray);
}	

void testApp::queueToJava(const List& list){
	toJavaMutex.lock();
	toJavaQueue.push(list);
	toJavaMutex.unlock();
}	

float extractFloatJobj(JNIEnv *env, jobject obj)
{
    jmethodID method  = env->GetMethodID(floatClass, "floatValue", "()F");
    if(method == NULL) return 0.0;
    return env->CallFloatMethod(obj, method);
}

int extractIntJobj(JNIEnv *env, jobject obj)
{
    jmethodID method  = env->GetMethodID(floatClass, "intValue", "()I");
    if(method == NULL) return 0;
    return env->CallIntMethod(obj, method);
}

#include <jni.h>

JNIEXPORT void JNICALL Java_net_metalu_PasteurQuiz_OFActivity_sendToPd (JNIEnv *env, jclass, jobjectArray array) {
	std::vector<Any> vec;
	unsigned int i, len = env->GetArrayLength(array);
	
	vec.push_back(string("SYSTEM"));
	
	for(i = 0 ; i < len ; i++ ) {
		jobject obj = env->GetObjectArrayElement(array, i);
		
		if(env->IsInstanceOf(obj, stringClass) == JNI_TRUE){
			const char * chr = env->GetStringUTFChars((jstring)obj, JNI_FALSE);
    		vec.push_back(string(chr));
    		env->ReleaseStringUTFChars((jstring)obj, chr);
    	}
    	else if(env->IsInstanceOf(obj, floatClass) == JNI_TRUE) {
    		vec.push_back(extractFloatJobj(env, obj));
    	}
    	else if(env->IsInstanceOf(obj, intClass) == JNI_TRUE) {
    		vec.push_back((float)extractIntJobj(env, obj));
    	}
	}
		
	pofBase::sendToPd(vec);
	//ofLogNotice("testApp")<< "JavaToPd: " << vec.size() << " args.";
}

