/******************************************************************************
 *
 * Computer Graphics Programming 2026 Project Template v2.3 (05/03/2026)
 *
 * Based on: Animation Controller v1.0 (11/04/2021)
 *
 * This template provides a basic FPS-limited render loop for an animated scene,
 * plus keyboard handling for smooth game-like control of an object such as a
 * character or vehicle.
 *
 * A simple static lighting setup is provided via initLights(), which is not
 * included in the animationalcontrol.c template. There are no other changes.
 *
 ******************************************************************************/

#include <Windows.h>
#include <freeglut.h>
#include <math.h>

 /******************************************************************************
  * Animation & Timing Setup
  ******************************************************************************/

  // Target frame rate (number of Frames Per Second).
#define TARGET_FPS 60				

// Ideal time each frame should be displayed for (in milliseconds).
const unsigned int FRAME_TIME = 1000 / TARGET_FPS;

// Frame time in fractional seconds.
// Note: This is calculated to accurately reflect the truncated integer value of
// FRAME_TIME, which is used for timing, rather than the more accurate fractional
// value we'd get if we simply calculated "FRAME_TIME_SEC = 1.0f / TARGET_FPS".
const float FRAME_TIME_SEC = (1000 / TARGET_FPS) / 1000.0f;

// Time we started preparing the current frame (in milliseconds since GLUT was initialized).
unsigned int frameStartTime = 0;

/******************************************************************************
 * Some Simple Definitions of Motion
 ******************************************************************************/

#define MOTION_NONE 0				// No motion.
#define MOTION_CLOCKWISE -1			// Clockwise rotation.
#define MOTION_ANTICLOCKWISE 1		// Anticlockwise rotation.
#define MOTION_BACKWARD -1			// Backward motion.
#define MOTION_FORWARD 1			// Forward motion.
#define MOTION_LEFT -1				// Leftward motion.
#define MOTION_RIGHT 1				// Rightward motion.
#define MOTION_DOWN -1				// Downward motion.
#define MOTION_UP 1					// Upward motion.

 // Represents the motion of an object on four axes (Yaw, Surge, Sway, and Heave).
 // 
 // You can use any numeric values, as specified in the comments for each axis. However,
 // the MOTION_ definitions offer an easy way to define a "unit" movement without using
 // magic numbers (e.g. instead of setting Surge = 1, you can set Surge = MOTION_FORWARD).
 //
typedef struct {
	int Yaw;		// Turn about the Z axis	[<0 = Clockwise, 0 = Stop, >0 = Anticlockwise]
	int Surge;		// Move forward or back		[<0 = Backward,	0 = Stop, >0 = Forward]
	int Sway;		// Move sideways (strafe)	[<0 = Left, 0 = Stop, >0 = Right]
	int Heave;		// Move vertically			[<0 = Down, 0 = Stop, >0 = Up]
} motionstate4_t;

/******************************************************************************
 * Keyboard Input Handling Setup
 ******************************************************************************/

// Represents the state of a single keyboard key.Represents the state of a single keyboard key.
typedef enum {
	KEYSTATE_UP = 0,	// Key is not pressed.
	KEYSTATE_DOWN		// Key is pressed down.
} keystate_t;

// Represents the states of a set of keys used to control an object's motion.
typedef struct {
	keystate_t MoveForward;
	keystate_t MoveBackward;
	keystate_t MoveLeft;
	keystate_t MoveRight;
	keystate_t MoveUp;
	keystate_t MoveDown;
	keystate_t TurnLeft;
	keystate_t TurnRight;
} motionkeys_t;

// Current state of all keys used to control our "player-controlled" object's motion.
motionkeys_t motionKeyStates = {
	KEYSTATE_UP, KEYSTATE_UP, KEYSTATE_UP, KEYSTATE_UP,
	KEYSTATE_UP, KEYSTATE_UP, KEYSTATE_UP, KEYSTATE_UP };

// How our "player-controlled" object should currently be moving, solely based on keyboard input.
//
// Note: this may not represent the actual motion of our object, which could be subject to
// other controls (e.g. mouse input) or other simulated forces (e.g. gravity).
motionstate4_t keyboardMotion = { MOTION_NONE, MOTION_NONE, MOTION_NONE, MOTION_NONE };

// Define all character keys used for input (add any new key definitions here).
// Note: USE ONLY LOWERCASE CHARACTERS HERE. The keyboard handler provided converts all
// characters typed by the user to lowercase, so the SHIFT key is ignored.

#define KEY_MOVE_FORWARD	'w'
#define KEY_MOVE_BACKWARD	's'
#define KEY_MOVE_LEFT		'a'
#define KEY_MOVE_RIGHT		'd'
#define KEY_RENDER_FILL		'l'
#define KEY_MOTOR_TOGGLE	' ' //spacebar
#define KEY_CAMERA_TOGGLE	'c'
#define KEY_EXIT			27 // Escape key.

// Define all GLUT special keys used for input (add any new key definitions here).

#define SP_KEY_MOVE_UP		GLUT_KEY_UP
#define SP_KEY_MOVE_DOWN	GLUT_KEY_DOWN
#define SP_KEY_TURN_LEFT	GLUT_KEY_LEFT
#define SP_KEY_TURN_RIGHT	GLUT_KEY_RIGHT

/******************************************************************************
 * GLUT Callback Prototypes
 ******************************************************************************/

void display(void);
void reshape(int width, int h);
void keyPressed(unsigned char key, int x, int y);
void specialKeyPressed(int key, int x, int y);
void keyReleased(unsigned char key, int x, int y);
void specialKeyReleased(int key, int x, int y);
void idle(void);

/******************************************************************************
 * Animation-Specific Function Prototypes (add your own here)
 ******************************************************************************/

void main(int argc, char **argv);
void init(void);
void think(void);
void initLights(void);
void drawGround();
void drawOctopus();
void drawTentacle();
void drawPropeller();
float getForwardX(float headingDeg);
float getForwardZ(float headingDeg);

/******************************************************************************
 * Animation-Specific Setup (Add your own definitions, constants, and globals here)
 ******************************************************************************/

#define GROUND_SIZE 100
#define TILE_SIZE 10
#define BODY_RADIUS 3.0f
#define EYE_RADIUS 0.5f
#define	TENTACLE_RADIUS 1.0f
#define TENTACLE_SPREAD 2.5f
#define PROPELLER_SIZE 1.5f
#define BEAK_RADIUS 0.5f
#define BEAK_LENGTH 0.8f
#define EYE_OFFSET_X 1.0f
#define EYE_OFFSET_Y 0.5f
#define EYE_DEPTH 0.83f
#define PROPELLER_SPEED_RPM 60.0f
#define FEET_SPEED_RPM 10.0f
#define RPM_TO_DEG_PER_SEC (360.0f / 60.0f)
#define OCTOPUS_MIN_Y (BODY_RADIUS + TENTACLE_RADIUS) // Lowest centre height before the feet touch the seabed
#define DEG_TO_RAD (3.14159265f / 180.0f)

#define MOVE_SPEED_MPS 5.0f              // Surge, sway and heave speed, metres per second
#define TURN_SPEED_DEG_PER_SEC 90.0f     // Yaw speed, degrees per second

#define CAMERA_DISTANCE_BEHIND 25.0f     // Metres behind the octopus
#define CAMERA_HEIGHT_ABOVE 8.0f         // Metres above the octopus centre
#define CAMERA_FOLLOW_RATE 0.0f          // How quickly the camera swings behind the octopus (per second)

// Current rotation angles in degrees, updated in think().
float propellerAngle = 0.0f;
float feetAngle = 0.0f;

// Octopus position (metres) and heading (degrees around Y, 0 = facing +Z).
float octopusX = 0.0f;
float octopusY = OCTOPUS_MIN_Y;
float octopusZ = 0.0f;
float octopusHeading = 0.0f;

// Motor on (1) or off (0). Feet, propeller and movement only run when on. Default off.
int motorEnabled = 0;

// Whether w was held last frame, so think() can detect the moment it's pressed.
keystate_t previousForwardKeyState = KEYSTATE_UP;

// Camera's own heading in degrees. Eases towards octopusHeading so turns are visible.
float cameraHeading = 0.0f;

// Camera view: behind the octopus (0) or in front looking at its face (1). Default behind.
int cameraFrontView = 0;

GLfloat lightPosition[] = { 5.0f, 20.0f, 5.0f, 1.0f };


// Render objects as filled polygons (1) or wireframes (0). Default filled.
int renderFillEnabled = 1;

/******************************************************************************
 * Entry Point (don't put anything except the main function here)
 ******************************************************************************/

void main(int argc, char **argv)
{
	// Initialize the OpenGL window.
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(1000, 800);
	glutCreateWindow("Animation");

	// Set up the scene.
	init();

	// Disable key repeat (keyPressed or specialKeyPressed will only be called once when a key is first pressed).
	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

	// Register GLUT callbacks.
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyPressed);
	glutSpecialFunc(specialKeyPressed);
	glutKeyboardUpFunc(keyReleased);
	glutSpecialUpFunc(specialKeyReleased);
	glutIdleFunc(idle);

	// Record when we started rendering the very first frame (which should happen after we call glutMainLoop).
	frameStartTime = (unsigned int)glutGet(GLUT_ELAPSED_TIME);

	// Enter the main drawing loop (this will never return).
	glutMainLoop();
}

/******************************************************************************
 * GLUT Callbacks (don't add any other functions here)
 ******************************************************************************/

 /*
	 Called when GLUT wants us to (re)draw the current animation frame.

	 Note: This function must not do anything to update the state of our simulated
	 world. Animation (moving or rotating things, responding to keyboard input,
	 etc.) should only be performed within the think() function provided below.
 */
void display(void)
{
	/*
		TEMPLATE: REPLACE THIS COMMENT WITH YOUR DRAWING CODE
		
		Separate reusable pieces of drawing code into functions, which you can add
		to the "Animation-Specific Functions" section below.
		
		Remember to add prototypes for any new functions to the "Animation-Specific
		Function Prototypes" section near the top of this template.
	*/
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// A negative distance puts the camera in front of the octopus, looking back at its face.
	float cameraDistance = cameraFrontView ? -CAMERA_DISTANCE_BEHIND : CAMERA_DISTANCE_BEHIND;
	// Tracking camera: behind and above the octopus, looking at its centre.
	gluLookAt(octopusX - getForwardX(cameraHeading) * cameraDistance,   // Eye X
		octopusY + CAMERA_HEIGHT_ABOVE,                                  // Eye Y
		octopusZ - getForwardZ(cameraHeading) * cameraDistance,          // Eye Z
		octopusX, octopusY, octopusZ,                                    // Target: the octopus
		0, 1, 0);                                                        // Up
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

	if (renderFillEnabled) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	drawGround();
	drawOctopus();
	glutSwapBuffers();
	
}

/*
	Called when the OpenGL window has been resized.
*/
void reshape(int width, int h)
{
	glViewport(0, 0, width, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (GLfloat)width / (GLfloat)h, 1.0, 100.0);
	glMatrixMode(GL_MODELVIEW);
}

/*
	Called each time a character key (e.g. a letter, number, or symbol) is pressed.
*/
void keyPressed(unsigned char key, int x, int y)
{
	switch (tolower(key)) {

	/*
		Keyboard-Controlled Motion Handler - DON'T CHANGE THIS SECTION

		Whenever one of our movement keys is pressed, we do two things:
		(1) Update motionKeyStates to record that the key is held down. We use
			this later in the keyReleased callback.
		(2) Update the relevant axis in keyboardMotion to set the new direction
			we should be moving in. The most recent key always "wins" (e.g. if
			you're holding down KEY_MOVE_LEFT then also pressed KEY_MOVE_RIGHT,
			our object will immediately start moving right).
	*/
	case KEY_MOVE_FORWARD:
		motionKeyStates.MoveForward = KEYSTATE_DOWN;
		keyboardMotion.Surge = MOTION_FORWARD;
		break;
	case KEY_MOVE_BACKWARD:
		motionKeyStates.MoveBackward = KEYSTATE_DOWN;
		keyboardMotion.Surge = MOTION_BACKWARD;
		break;
	case KEY_MOVE_LEFT:
		motionKeyStates.MoveLeft = KEYSTATE_DOWN;
		keyboardMotion.Sway = MOTION_LEFT;
		break;
	case KEY_MOVE_RIGHT:
		motionKeyStates.MoveRight = KEYSTATE_DOWN;
		keyboardMotion.Sway = MOTION_RIGHT;
		break;

	/*
		Other Keyboard Functions (add any new character key controls here)

		Rather than using literals (e.g. "t" for spotlight), create a new KEY_
		definition in the "Keyboard Input Handling Setup" section of this file.
		For example, refer to the existing keys used here (KEY_MOVE_FORWARD,
		KEY_MOVE_LEFT, KEY_EXIT, etc).
	*/
	case KEY_RENDER_FILL:
		renderFillEnabled = !renderFillEnabled;
		break;
	case KEY_MOTOR_TOGGLE:
		motorEnabled = !motorEnabled;
		break;
	case KEY_CAMERA_TOGGLE:
		cameraFrontView = !cameraFrontView;
		break;
	case KEY_EXIT:
		exit(0);
		break;
	}
}

/*
	Called each time a "special" key (e.g. an arrow key) is pressed.
*/
void specialKeyPressed(int key, int x, int y)
{
	switch (key) {

	/*
		Keyboard-Controlled Motion Handler - DON'T CHANGE THIS SECTION

		This works as per the motion keys in keyPressed.
	*/
	case SP_KEY_MOVE_UP:
		motionKeyStates.MoveUp = KEYSTATE_DOWN;
		keyboardMotion.Heave = MOTION_UP;
		break;
	case SP_KEY_MOVE_DOWN:
		motionKeyStates.MoveDown = KEYSTATE_DOWN;
		keyboardMotion.Heave = MOTION_DOWN;
		break;
	case SP_KEY_TURN_LEFT:
		motionKeyStates.TurnLeft = KEYSTATE_DOWN;
		keyboardMotion.Yaw = MOTION_ANTICLOCKWISE;
		break;
	case SP_KEY_TURN_RIGHT:
		motionKeyStates.TurnRight = KEYSTATE_DOWN;
		keyboardMotion.Yaw = MOTION_CLOCKWISE;
		break;

	/*
		Other Keyboard Functions (add any new special key controls here)

		Rather than directly using the GLUT constants (e.g. GLUT_KEY_F1), create
		a new SP_KEY_ definition in the "Keyboard Input Handling Setup" section of
		this file. For example, refer to the existing keys used here (SP_KEY_MOVE_UP,
		SP_KEY_TURN_LEFT, etc).
	*/
	}
}

/*
	Called each time a character key (e.g. a letter, number, or symbol) is released.
*/
void keyReleased(unsigned char key, int x, int y)
{
	switch (tolower(key)) {

	/*
		Keyboard-Controlled Motion Handler - DON'T CHANGE THIS SECTION

		Whenever one of our movement keys is released, we do two things:
		(1) Update motionKeyStates to record that the key is no longer held down;
			we need to know when we get to step (2) below.
		(2) Update the relevant axis in keyboardMotion to set the new direction
			we should be moving in. This gets a little complicated to ensure
			the controls work smoothly. When the user releases a key that moves
			in one direction (e.g. KEY_MOVE_RIGHT), we check if its "opposite"
			key (e.g. KEY_MOVE_LEFT) is pressed down. If it is, we begin moving
			in that direction instead. Otherwise, we just stop moving.
	*/
	case KEY_MOVE_FORWARD:
		motionKeyStates.MoveForward = KEYSTATE_UP;
		keyboardMotion.Surge = (motionKeyStates.MoveBackward == KEYSTATE_DOWN) ? MOTION_BACKWARD : MOTION_NONE;
		break;
	case KEY_MOVE_BACKWARD:
		motionKeyStates.MoveBackward = KEYSTATE_UP;
		keyboardMotion.Surge = (motionKeyStates.MoveForward == KEYSTATE_DOWN) ? MOTION_FORWARD : MOTION_NONE;
		break;
	case KEY_MOVE_LEFT:
		motionKeyStates.MoveLeft = KEYSTATE_UP;
		keyboardMotion.Sway = (motionKeyStates.MoveRight == KEYSTATE_DOWN) ? MOTION_RIGHT : MOTION_NONE;
		break;
	case KEY_MOVE_RIGHT:
		motionKeyStates.MoveRight = KEYSTATE_UP;
		keyboardMotion.Sway = (motionKeyStates.MoveLeft == KEYSTATE_DOWN) ? MOTION_LEFT : MOTION_NONE;
		break;

	/*
		Other Keyboard Functions (add any new character key controls here)

		Note: If you only care when your key is first pressed down, you don't have to
		add anything here. You only need to put something in keyReleased if you care
		what happens when the user lets go, like we do with our movement keys above.
		For example: if you wanted a spotlight to come on while you held down "t", you
		would need to set a flag to turn the spotlight on in keyPressed, and update the
		flag to turn it off in keyReleased.
	*/
	}
}

/*
	Called each time a "special" key (e.g. an arrow key) is released.
*/
void specialKeyReleased(int key, int x, int y)
{
	switch (key) {
	/*
		Keyboard-Controlled Motion Handler - DON'T CHANGE THIS SECTION

		This works as per the motion keys in keyReleased.
	*/
	case SP_KEY_MOVE_UP:
		motionKeyStates.MoveUp = KEYSTATE_UP;
		keyboardMotion.Heave = (motionKeyStates.MoveDown == KEYSTATE_DOWN) ? MOTION_DOWN : MOTION_NONE;
		break;
	case SP_KEY_MOVE_DOWN:
		motionKeyStates.MoveDown = KEYSTATE_UP;
		keyboardMotion.Heave = (motionKeyStates.MoveUp == KEYSTATE_DOWN) ? MOTION_UP : MOTION_NONE;
		break;
	case SP_KEY_TURN_LEFT:
		motionKeyStates.TurnLeft = KEYSTATE_UP;
		keyboardMotion.Yaw = (motionKeyStates.TurnRight == KEYSTATE_DOWN) ? MOTION_CLOCKWISE : MOTION_NONE;
		break;
	case SP_KEY_TURN_RIGHT:
		motionKeyStates.TurnRight = KEYSTATE_UP;
		keyboardMotion.Yaw = (motionKeyStates.TurnLeft == KEYSTATE_DOWN) ? MOTION_ANTICLOCKWISE : MOTION_NONE;
		break;

	/*
		Other Keyboard Functions (add any new special key controls here)

		As per keyReleased, you only need to handle the key here if you want something
		to happen when the user lets go. If you just want something to happen when the
		key is first pressed, add you code to specialKeyPressed instead.
	*/
	}
}

/*
	Called by GLUT when it's not rendering a frame.

	Note: We use this to handle animation and timing. You shouldn't need to modify
	this callback at all. Instead, place your animation logic (e.g. moving or rotating
	things) within the think() method provided with this template.
*/
void idle(void)
{
	// Wait until it's time to render the next frame.

	unsigned int frameTimeElapsed = (unsigned int)glutGet(GLUT_ELAPSED_TIME) - frameStartTime;
	if (frameTimeElapsed < FRAME_TIME)
	{
		// This frame took less time to render than the ideal FRAME_TIME: we'll suspend this thread for the remaining time,
		// so we're not taking up the CPU until we need to render another frame.
		unsigned int timeLeft = FRAME_TIME - frameTimeElapsed;
		Sleep(timeLeft);
	}

	// Begin processing the next frame.

	frameStartTime = glutGet(GLUT_ELAPSED_TIME); // Record when we started work on the new frame.

	think(); // Update our simulated world before the next call to display().

	glutPostRedisplay(); // Tell OpenGL there's a new frame ready to be drawn.
}

/******************************************************************************
 * Animation-Specific Functions (Add your own functions at the end of this section)
 ******************************************************************************/

 /*
	 Initialise OpenGL and set up our scene before we begin the render loop.
 */
void init(void)
{
	initLights();
	
	// Anything that relies on lighting or specifies normals must be initialised after initLights.
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0, 0.1, 0.3, 1.0);
}

/*
	Advance our animation by FRAME_TIME milliseconds.

	Note: Our template's GLUT idle() callback calls this once before each new
	frame is drawn, EXCEPT the very first frame drawn after our application
	starts. Any setup required before the first frame is drawn should be placed
	in init().
*/
void think(void)
{
	// w switches the motor on/off (per spec). Its handler is in a DON'T CHANGE section,
	// so detect the moment it goes from up to down here instead.
	if (motionKeyStates.MoveForward == KEYSTATE_DOWN && previousForwardKeyState == KEYSTATE_UP) {
		motorEnabled = !motorEnabled;
	}
	previousForwardKeyState = motionKeyStates.MoveForward;

	if (motorEnabled) {
		propellerAngle += PROPELLER_SPEED_RPM * RPM_TO_DEG_PER_SEC * FRAME_TIME_SEC;
		if (propellerAngle >= 360.0f) {
			propellerAngle -= 360.0f;
		}

		feetAngle -= FEET_SPEED_RPM * RPM_TO_DEG_PER_SEC * FRAME_TIME_SEC;
		if (feetAngle < 0.0f) {
			feetAngle += 360.0f;
		}
	}
	/*
		TEMPLATE: REPLACE THIS COMMENT WITH YOUR ANIMATION/SIMULATION CODE

		In this function, we update all the variables that control the animated
		parts of our simulated world. For example: if you have a moving box, this is
		where you update its coordinates to make it move. If you have something that
		spins around, here's where you update its angle.

		NOTHING CAN BE DRAWN IN HERE: you can only update the variables that control
		how everything will be drawn later in display().

		How much do we move or rotate things? Because we use a fixed frame rate, we
		assume there's always FRAME_TIME milliseconds between drawing each frame. So,
		every time think() is called, we need to work out how far things should have
		moved, rotated, or otherwise changed in that period of time.

		Movement example:
		* Let's assume a distance of 1.0 GL units is 1 metre.
		* Let's assume we want something to move 2 metres per second on the x axis
		* Each frame, we'd need to update its position like this:
			x += 2 * (FRAME_TIME / 1000.0f)
		* Note that we have to convert FRAME_TIME to seconds. We can skip this by
		  using a constant defined earlier in this template:
			x += 2 * FRAME_TIME_SEC;

		Rotation example:
		* Let's assume we want something to do one complete 360-degree rotation every
		  second (i.e. 60 Revolutions Per Minute, or RPM).
		* Each frame, we'd need to update our object's angle like this (we'll use the
		  FRAME_TIME_SEC constant as per the example above):
			a += 360 * FRAME_TIME_SEC;

		This works for any type of "per second" change: just multiply the amount you'd
		want to move in a full second by FRAME_TIME_SEC, and add or subtract that
		from whatever variable you're updating.

		You can use this same approach to animate other things like color, opacity,
		brightness of lights, etc.
	*/

	/*
		Keyboard motion handler: complete this section to make your "player-controlled"
		object respond to keyboard input.
	*/
	if (keyboardMotion.Yaw != MOTION_NONE) {
		// Turn left (Yaw > 0) or right (Yaw < 0), keeping the heading within 0 to 360. Allowed with the motor off, as in the demo.
		octopusHeading += TURN_SPEED_DEG_PER_SEC * keyboardMotion.Yaw * FRAME_TIME_SEC;
		if (octopusHeading >= 360.0f) {
			octopusHeading -= 360.0f;
		}
		if (octopusHeading < 0.0f) {
			octopusHeading += 360.0f;
		}
	}

	// Surge: w/spacebar switch forward motion on and off, so it swims forward whenever the motor is on.
	if (motorEnabled) {
		octopusX += getForwardX(octopusHeading) * MOVE_SPEED_MPS * MOTION_FORWARD * FRAME_TIME_SEC;
		octopusZ += getForwardZ(octopusHeading) * MOVE_SPEED_MPS * MOTION_FORWARD * FRAME_TIME_SEC;
	}

	if (keyboardMotion.Sway != MOTION_NONE) {
		// Strafe right (Sway > 0) or left (Sway < 0). Right is forward swapped with one sign flipped: (-fZ, fX).
		if (motorEnabled) {
			octopusX -= getForwardZ(octopusHeading) * MOVE_SPEED_MPS * keyboardMotion.Sway * FRAME_TIME_SEC;
			octopusZ += getForwardX(octopusHeading) * MOVE_SPEED_MPS * keyboardMotion.Sway * FRAME_TIME_SEC;
		}
	}

	if (keyboardMotion.Heave != MOTION_NONE) {
		// Move up (Heave > 0) or down (Heave < 0), never below the seabed.
		if (motorEnabled) {
			octopusY += MOVE_SPEED_MPS * keyboardMotion.Heave * FRAME_TIME_SEC;
			if (octopusY < OCTOPUS_MIN_Y) {
				octopusY = OCTOPUS_MIN_Y;
			}
		}
	}

	// Ease the camera heading towards the octopus heading, taking the shortest way round.
	float headingDifference = octopusHeading - cameraHeading;
	if (headingDifference > 180.0f) {
		headingDifference -= 360.0f;
	}
	if (headingDifference < -180.0f) {
		headingDifference += 360.0f;
	}
	cameraHeading += headingDifference * CAMERA_FOLLOW_RATE * FRAME_TIME_SEC;
	if (cameraHeading >= 360.0f) {
		cameraHeading -= 360.0f;
	}
	if (cameraHeading < 0.0f) {
		cameraHeading += 360.0f;
	}
}

/*
	Initialise OpenGL lighting before we begin the render loop.
	
	Note (advanced): If you're using dynamic lighting (e.g. lights that move around, turn on or
	off, or change colour) you may want to replace this with a drawLights function that gets called
	at the beginning of display() instead of init().
*/
void initLights(void)
{
	// Simple lighting setup
	GLfloat globalAmbient[] = { 0.4f, 0.4f, 0.4f, 1 };
	GLfloat ambientLight[] = { 0, 0, 0, 1 };
	GLfloat diffuseLight[] = { 1, 1, 1, 1 };
	GLfloat specularLight[] = { 1, 1, 1, 1 };
	
	// Configure global ambient lighting.
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
	
	// Configure Light 0.
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
	
	// Enable lighting
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	
	// Make GL normalize the normal vectors we supply.
	glEnable(GL_NORMALIZE);
	
	// Enable use of simple GL colours as materials.
	glEnable(GL_COLOR_MATERIAL);
}

void drawGround() {
	// Draw a ground with a green color with tiles for lighting 
	glColor3f(0.3f, 0.8f, 0.3f);

	for (int x = -GROUND_SIZE / 2; x < GROUND_SIZE / 2; x += TILE_SIZE) {
		for (int z = -GROUND_SIZE / 2; z < GROUND_SIZE / 2; z += TILE_SIZE) {
			glBegin(GL_QUADS);
			glNormal3f(0.0f, 1.0f, 0.0f); // Upward normal
			glVertex3f(x, 0.0f, z);
			glVertex3f(x, 0.0f, z + TILE_SIZE);
			glVertex3f(x + TILE_SIZE, 0.0f, z + TILE_SIZE);
			glVertex3f(x + TILE_SIZE, 0.0f, z);
			glEnd();
		}
	}
}

void drawTentacle() {
	glPushMatrix();
	glColor3f(0.0f, 0.8f, 0.8f);
	glutSolidSphere(TENTACLE_RADIUS, 6, 6);
	glPopMatrix();
}

void drawPropeller() {
	// Two blades meeting at the origin, flat in the XY plane so a Z rotation spins it in place.
	GLfloat vertices[] = {
		// Top blade
		0.0f, 0.0f, 0.0f,
		-PROPELLER_SIZE / 2, PROPELLER_SIZE, 0.0f,
		PROPELLER_SIZE / 2, PROPELLER_SIZE, 0.0f,
		// Bottom blade
		0.0f, 0.0f, 0.0f,
		PROPELLER_SIZE / 2, -PROPELLER_SIZE, 0.0f,
		-PROPELLER_SIZE / 2, -PROPELLER_SIZE, 0.0f
	};

	glColor3f(1.0f, 0.5f, 0.0f);
	glNormal3f(0.0f, 0.0f, -1.0f); // Backward normal
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void drawOctopus() {
	glPushMatrix();
	glTranslatef(octopusX, octopusY, octopusZ);
	glRotatef(octopusHeading, 0, 1, 0);
	glColor3f(0.0f, 0.8f, 0.8f);
	glutSolidSphere(BODY_RADIUS, 20, 20);

	glPushMatrix();
	glTranslatef(EYE_OFFSET_X, EYE_OFFSET_Y, BODY_RADIUS * EYE_DEPTH);
	glColor3f(0.0f, 0.0f, 0.0f);
	glutSolidSphere(EYE_RADIUS, 10, 10);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-EYE_OFFSET_X, EYE_OFFSET_Y, BODY_RADIUS * EYE_DEPTH);
	glColor3f(0.0f, 0.0f, 0.0f);
	glutSolidSphere(EYE_RADIUS, 10, 10);
	glPopMatrix();

	// Propeller on the back (-Z)
	glPushMatrix();
	glTranslatef(0, 0, -BODY_RADIUS);
	glRotatef(propellerAngle, 0, 0, 1);
	drawPropeller();
	glPopMatrix();

	// Beak on the underside, pointing down
	glPushMatrix();
	glTranslatef(0, -BODY_RADIUS, 0);
	glRotatef(90.0f, 1, 0, 0);
	glColor3f(1.0f, 0.6f, 0.0f);
	glutSolidCone(BEAK_RADIUS, BEAK_LENGTH, 10, 10);
	glPopMatrix();

	for (int i = 0; i < 8; i++) {
		glPushMatrix();
		glRotatef(feetAngle + i * 45.0f, 0, 1, 0);   // spread around Y
		glTranslatef(TENTACLE_SPREAD, -BODY_RADIUS, 0);        // offset to bottom edge
		drawTentacle();                   // draw one tentacle here
		glPopMatrix();
	}

	glPopMatrix();
}

/*
	Unit vector on the ground (X and Z) for a heading in degrees.
	Heading 0 faces +Z, heading 90 faces +X. Converts degrees to radians for sinf/cosf.
*/
float getForwardX(float headingDeg) {
	return sinf(headingDeg * DEG_TO_RAD);
}

float getForwardZ(float headingDeg) {
	return cosf(headingDeg * DEG_TO_RAD);
}

/****************************************2026****************************************/