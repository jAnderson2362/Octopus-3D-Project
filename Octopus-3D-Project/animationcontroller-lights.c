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

#define _CRT_SECURE_NO_WARNINGS   // Allow standard fopen; MSVC's fopen_s is Windows-only

#include <Windows.h>
#include <freeglut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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
#define KEY_MAIN_LIGHT_TOGGLE	'1'
#define KEY_SPOTLIGHT_TOGGLE	'2'
#define KEY_HILLS_TOGGLE	'h'
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
void setMaterial(GLfloat red, GLfloat green, GLfloat blue, GLfloat specular, GLfloat shininess);
void drawLights(void);
int readInt32(const unsigned char* bytes);
unsigned char* loadBMP(const char* filename, int* width, int* height);
GLuint loadTexture(const char* filename);
float groundHeight(float x, float z);
void drawGroundVertex(float x, float z);
void drawGroundSquare(int centreX, int centreZ, int size, int tileSize, int holeSize);

/******************************************************************************
 * Animation-Specific Setup (Add your own definitions, constants, and globals here)
 ******************************************************************************/

#define GROUND_SIZE 100
#define TILE_SIZE 1
#define FAR_GROUND_SIZE 800       // Coarse ground out towards the horizon, metres across
#define FAR_TILE_SIZE 10          // Big far tiles: the detail can't be seen that far away
#define FAR_GROUND_DROP 1.0f      // Far ground sits this far lower, so where it overlaps the near ground it stays hidden underneath
#define BODY_RADIUS 3.0f
#define EYE_RADIUS 0.5f
#define	TENTACLE_RADIUS 1.0f
#define TENTACLE_SPREAD 2.5f
#define TENTACLE_COUNT 8
#define PROPELLER_SIZE 1.5f
#define BEAK_RADIUS 0.5f
#define BEAK_LENGTH 0.8f
#define EYE_OFFSET_X 1.0f
#define EYE_OFFSET_Y 0.5f
#define EYE_DEPTH 0.83f
#define PROPELLER_SPEED_RPM 60.0f
#define FEET_SPEED_RPM 40.0f
#define RPM_TO_DEG_PER_SEC (360.0f / 60.0f)
#define OCTOPUS_MIN_Y (BODY_RADIUS + TENTACLE_RADIUS) // Lowest centre height before the feet touch the seabed
#define DEG_TO_RAD (3.14159265f / 180.0f)

#define MOVE_SPEED_MPS 5.0f              // Surge, sway and heave speed, metres per second
#define TURN_SPEED_DEG_PER_SEC 90.0f     // Yaw speed, degrees per second

#define CAMERA_DISTANCE_BEHIND 25.0f     // Metres behind the octopus
#define CAMERA_HEIGHT_ABOVE 8.0f         // Metres above the octopus centre
#define CAMERA_FOLLOW_RATE 3.0f          // How quickly the camera swings behind the octopus (per second)

#define WET_SPECULAR 0.8f                // Octopus: strong highlights, looks wet
#define WET_SHININESS 60.0f              // Octopus: small, sharp highlights (0 to 128)
#define MATTE_SPECULAR 0.0f              // Seabed, beak, propeller: no highlights
#define MATTE_SHININESS 0.0f

#define SPOTLIGHT_HEIGHT 7.0f                    // Metres above the octopus centre
#define SPOTLIGHT_FORWARD 2.0f                   // Metres in front of the octopus centre
#define SPOTLIGHT_CUTOFF_DEG 40.0f               // Half the cone angle
#define SPOTLIGHT_EXPONENT 5.0f                  // How focused the beam is (higher = brighter centre, softer edge)
#define SPOTLIGHT_SWEEP_ANGLE 30.0f              // How far the beam swings to each side, degrees
#define SPOTLIGHT_SWEEP_SPEED_DEG_PER_SEC 90.0f  // Sweep cycle speed (360 = one full left-right-left cycle per second)

#define SAND_TEXTURE_FILE "sand.bmp"
#define SKIN_TEXTURE_FILE "skin.bmp"
#define SEABED_TEXTURE_SIZE 20.0f                 // Metres of seabed covered by one copy of the sand image

#define FLAT_FOG_START 30.0f                     // Flat grid: fog begins just past the octopus (about 26 m from the camera)
#define FLAT_FOG_END 90.0f                       // Flat grid: mostly fogged by the grid's far edge, so it fades instead of cutting off
#define HILLS_FOG_START 100.0f                   // Hills: fog begins further out, so more of the hills can be seen
#define HILLS_FOG_END 340.0f                     // Hills: fully fogged before the edge of the far ground can be seen

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

// Main light: directional (w = 0), shining down like sunlight through water.
// For a directional light, x, y, z point TOWARDS the light: here from above, slightly to one side and in front.
GLfloat mainLightDirection[] = { 0.3f, 1.0f, 0.4f, 0.0f };

// Sea colour, used for both the background (glClearColor) and the fog, so they always match.
GLfloat waterColour[] = { 0.0f, 0.1f, 0.3f, 1.0f };

// Spotlight sweep phase in degrees. think() advances it, and drawLights() turns it into a side-to-side swing.
float spotlightSweepPhase = 0.0f;

// Light switches: on (1) or off (0). Both start on.
int mainLightEnabled = 1;
int spotlightEnabled = 1;

// Render objects as filled polygons (1) or wireframes (0). Default filled.
int renderFillEnabled = 1;

// Seabed: flat grid at the origin (0, per spec) or endless hills (1, extra feature). Default flat.
int hillsEnabled = 0;

// Texture IDs from glGenTextures, set once in init(). 0 means not loaded.
GLuint sandTexture = 0;
GLuint skinTexture = 0;

// Quadric for the body sphere. Unlike glutSolidSphere, it can generate texture coordinates.
GLUquadric* bodyQuadric = NULL;

// Seabed display list: compiled once and replayed every frame. Only rebuilt when the ground
// moves (hills, every FAR_TILE_SIZE metres) or the seabed mode changes.
GLuint groundList = 0;
int groundListCentreX = 0;
int groundListCentreZ = 0;
int groundListHills = -1;      // Seabed mode the list was built for (-1 = not built yet)

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
	drawLights();

	// Fog distances depend on the seabed: close in for the small flat grid, far out for the endless hills.
	if (hillsEnabled) {
		glFogf(GL_FOG_START, HILLS_FOG_START);
		glFogf(GL_FOG_END, HILLS_FOG_END);
	}
	else {
		glFogf(GL_FOG_START, FLAT_FOG_START);
		glFogf(GL_FOG_END, FLAT_FOG_END);
	}

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
	gluPerspective(60, (GLfloat)width / (GLfloat)h, 1.0, 400.0);
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
	case KEY_MAIN_LIGHT_TOGGLE:
		mainLightEnabled = !mainLightEnabled;
		break;
	case KEY_SPOTLIGHT_TOGGLE:
		spotlightEnabled = !spotlightEnabled;
		break;
	case KEY_HILLS_TOGGLE:
		hillsEnabled = !hillsEnabled;
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
	glClearColor(waterColour[0], waterColour[1], waterColour[2], waterColour[3]);

	// Fog in the water colour, so distant things fade into the background. Distances are set each frame in display().
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogfv(GL_FOG_COLOR, waterColour);
	glEnable(GL_FOG);

	// Textures are loaded once here, never in display().
	sandTexture = loadTexture(SAND_TEXTURE_FILE);
	skinTexture = loadTexture(SKIN_TEXTURE_FILE);

	bodyQuadric = gluNewQuadric();
	gluQuadricTexture(bodyQuadric, GL_TRUE);   // Generate texture coordinates for the sphere

	groundList = glGenLists(1);   // Reserve one display list ID for the seabed
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

	// Propeller only spins when the motor is on (forward thrust).
	if (motorEnabled) {
		propellerAngle += PROPELLER_SPEED_RPM * RPM_TO_DEG_PER_SEC * FRAME_TIME_SEC;
		if (propellerAngle >= 360.0f) {
			propellerAngle -= 360.0f;
		}
	}

	// Feet spin when the motor is on or when turning. While turning they spin the same way as the turn,
	// otherwise clockwise (opposite to the propeller).
	if (motorEnabled || keyboardMotion.Yaw != MOTION_NONE) {
		int feetDirection = MOTION_CLOCKWISE;
		if (keyboardMotion.Yaw != MOTION_NONE) {
			feetDirection = keyboardMotion.Yaw;
		}

		feetAngle += FEET_SPEED_RPM * RPM_TO_DEG_PER_SEC * feetDirection * FRAME_TIME_SEC;
		if (feetAngle >= 360.0f) {
			feetAngle -= 360.0f;
		}
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
		/* TEMPLATE: Turn your object right (clockwise) if .Yaw < 0, or left (anticlockwise) if .Yaw > 0 */
		// Allowed with the motor off, as in the demo. Heading kept within 0 to 360.
		octopusHeading += TURN_SPEED_DEG_PER_SEC * keyboardMotion.Yaw * FRAME_TIME_SEC;
		if (octopusHeading >= 360.0f) {
			octopusHeading -= 360.0f;
		}
		if (octopusHeading < 0.0f) {
			octopusHeading += 360.0f;
		}
	}
	if (keyboardMotion.Surge != MOTION_NONE) {
		/* TEMPLATE: Move your object backward if .Surge < 0, or forward if .Surge > 0 */
		// Not used: per the spec, w toggles forward motion on/off instead of being held (see below).
	}

	// Surge: w/spacebar switch forward motion on and off, so it swims forward whenever the motor is on.
	if (motorEnabled) {
		octopusX += getForwardX(octopusHeading) * MOVE_SPEED_MPS * FRAME_TIME_SEC;
		octopusZ += getForwardZ(octopusHeading) * MOVE_SPEED_MPS * FRAME_TIME_SEC;
	}

	if (keyboardMotion.Sway != MOTION_NONE) {
		/* TEMPLATE: Move (strafe) your object left if .Sway < 0, or right if .Sway > 0 */
		// Right is forward swapped with one sign flipped: (-fZ, fX).
		if (motorEnabled) {
			octopusX -= getForwardZ(octopusHeading) * MOVE_SPEED_MPS * keyboardMotion.Sway * FRAME_TIME_SEC;
			octopusZ += getForwardX(octopusHeading) * MOVE_SPEED_MPS * keyboardMotion.Sway * FRAME_TIME_SEC;
		}
	}
	if (keyboardMotion.Heave != MOTION_NONE) {
		/* TEMPLATE: Move your object down if .Heave < 0, or up if .Heave > 0 */
		if (motorEnabled) {
			octopusY += MOVE_SPEED_MPS * keyboardMotion.Heave * FRAME_TIME_SEC;
		}
	}

	// Never below the seabed: keep the feet above the ground under the octopus, even when swimming into a hill.
	float lowestY = groundHeight(octopusX, octopusZ) + OCTOPUS_MIN_Y;
	if (octopusY < lowestY) {
		octopusY = lowestY;
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

	// Spotlight sweep: always running, so the beam animates even when the octopus is parked.
	spotlightSweepPhase += SPOTLIGHT_SWEEP_SPEED_DEG_PER_SEC * FRAME_TIME_SEC;
	if (spotlightSweepPhase >= 360.0f) {
		spotlightSweepPhase -= 360.0f;
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
	GLfloat globalAmbient[] = { 0.10f, 0.12f, 0.18f, 1 };   // Dim and blue, so the spotlight stands out
	GLfloat ambientLight[] = { 0, 0, 0, 1 };
	GLfloat diffuseLight[] = { 0.55f, 0.75f, 0.85f, 1 };     // Blue-green, like daylight filtered through sea water
	GLfloat specularLight[] = { 0.7f, 0.85f, 0.9f, 1 };      // Slightly blue highlights
	GLfloat spotDiffuse[] = { 1.0f, 0.9f, 0.6f, 1 };         // Warm yellow beam, stands out against the blue water
	GLfloat spotSpecular[] = { 1.0f, 1.0f, 0.9f, 1 };
	
	// Configure global ambient lighting.
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
	
	// Configure Light 0.
	glLightfv(GL_LIGHT0, GL_POSITION, mainLightDirection);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);

	// Configure Light 1: the sweeping spotlight. Its position and direction are set every frame in drawLights().
	glLightfv(GL_LIGHT1, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, spotDiffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, spotSpecular);
	glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, SPOTLIGHT_CUTOFF_DEG);
	glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, SPOTLIGHT_EXPONENT);
	
	// Enable lighting
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	
	// Make GL normalize the normal vectors we supply.
	glEnable(GL_NORMALIZE);

	// Enable use of simple GL colours as materials.
	// Removed: every object now sets a real material with setMaterial().
	
	// Light both sides of flat shapes (the propeller), not just the front.
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
}

void drawGround() {
	// Sand texture over a white material, so lighting shades the image without tinting it.
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, sandTexture);
	setMaterial(1.0f, 1.0f, 1.0f, MATTE_SPECULAR, MATTE_SHININESS);

	// Flat: the spec's grid, centred at the origin. Hills: centred on the octopus, snapped to whole
	// far tiles, so it never runs out and the near square's edges line up with the far tiles.
	int centreX = 0;
	int centreZ = 0;
	if (hillsEnabled) {
		centreX = (int)floorf(octopusX / FAR_TILE_SIZE) * FAR_TILE_SIZE;
		centreZ = (int)floorf(octopusZ / FAR_TILE_SIZE) * FAR_TILE_SIZE;
	}

	// Rebuild the display list only when the seabed mode or position has changed since it was built.
	if (hillsEnabled != groundListHills || centreX != groundListCentreX || centreZ != groundListCentreZ) {
		glNewList(groundList, GL_COMPILE);
		if (hillsEnabled) {
			// Near: small tiles, so the spotlight circle looks round.
			drawGroundSquare(centreX, centreZ, GROUND_SIZE, TILE_SIZE, 0);
			// Far: big tiles out towards the horizon. The hole is one far tile smaller than the near ground, so the
			// two overlap with no cracks, and the far ground is lowered slightly so it stays hidden under the near ground.
			glPushMatrix();
			glTranslatef(0.0f, -FAR_GROUND_DROP, 0.0f);
			drawGroundSquare(centreX, centreZ, FAR_GROUND_SIZE, FAR_TILE_SIZE, GROUND_SIZE - 2 * FAR_TILE_SIZE);
			glPopMatrix();
		}
		else {
			// Spec: a flat grid of squares on the XZ plane, centred at the origin.
			drawGroundSquare(0, 0, GROUND_SIZE, TILE_SIZE, 0);
		}
		glEndList();

		groundListHills = hillsEnabled;
		groundListCentreX = centreX;
		groundListCentreZ = centreZ;
	}

	glCallList(groundList);

	glDisable(GL_TEXTURE_2D);
}

void drawTentacle() {
	glPushMatrix();
	// Same skin texture as the body.
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, skinTexture);
	setMaterial(1.0f, 1.0f, 1.0f, WET_SPECULAR, WET_SHININESS);
	gluSphere(bodyQuadric, TENTACLE_RADIUS, 6, 6);
	glDisable(GL_TEXTURE_2D);
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

	setMaterial(1.0f, 0.5f, 0.0f, MATTE_SPECULAR, MATTE_SHININESS);
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
	// Skin texture on the body. White material so the texture isn't tinted cyan.
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, skinTexture);
	setMaterial(1.0f, 1.0f, 1.0f, WET_SPECULAR, WET_SHININESS);
	gluSphere(bodyQuadric, BODY_RADIUS, 20, 20);
	glDisable(GL_TEXTURE_2D);;

	glPushMatrix();
	glTranslatef(EYE_OFFSET_X, EYE_OFFSET_Y, BODY_RADIUS * EYE_DEPTH);
	setMaterial(0.0f, 0.0f, 0.0f, WET_SPECULAR, WET_SHININESS);
	glutSolidSphere(EYE_RADIUS, 10, 10);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-EYE_OFFSET_X, EYE_OFFSET_Y, BODY_RADIUS * EYE_DEPTH);
	setMaterial(0.0f, 0.0f, 0.0f, WET_SPECULAR, WET_SHININESS);
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
	setMaterial(1.0f, 0.6f, 0.0f, MATTE_SPECULAR, MATTE_SHININESS);
	glutSolidCone(BEAK_RADIUS, BEAK_LENGTH, 10, 10);
	glPopMatrix();

	for (int i = 0; i < TENTACLE_COUNT; i++) {
		glPushMatrix();
		glRotatef(feetAngle + i * (360.0f / TENTACLE_COUNT), 0, 1, 0);   // spread evenly around Y
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

/*
	Set the material for everything drawn after this call.
	The colour is used for both ambient and diffuse. Specular is a grey level (0 to 1)
	and shininess controls how tight the highlight is (0 to 128).
*/
void setMaterial(GLfloat red, GLfloat green, GLfloat blue, GLfloat specular, GLfloat shininess) {
	GLfloat colour[] = { red, green, blue, 1.0f };
	GLfloat specularColour[] = { specular, specular, specular, 1.0f };

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularColour);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

/*
	Set light positions and directions for this frame. Called after gluLookAt and before drawing,
	so every object is lit by this frame's lights.
*/
void drawLights(void) {
	GLfloat spotPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f };   // w = 1: a light at a point, needed for a spotlight
	GLfloat spotDirection[] = { 0.0f, -1.0f, 0.0f };       // Straight down, before the sweep is applied

	// Switch each light on or off to match its toggle key.
	if (mainLightEnabled) {
		glEnable(GL_LIGHT0);
	}
	else {
		glDisable(GL_LIGHT0);
	}
	if (spotlightEnabled) {
		glEnable(GL_LIGHT1);
	}
	else {
		glDisable(GL_LIGHT1);
	}

	// Main light: set after gluLookAt so its direction is fixed in the world, not to the camera.
	glLightfv(GL_LIGHT0, GL_POSITION, mainLightDirection);

	// Spotlight: same transforms as the octopus, so it moves and turns with it.
	glPushMatrix();
	glTranslatef(octopusX, octopusY, octopusZ);
	glRotatef(octopusHeading, 0, 1, 0);
	glTranslatef(0.0f, SPOTLIGHT_HEIGHT, SPOTLIGHT_FORWARD);
	glRotatef(SPOTLIGHT_SWEEP_ANGLE * sinf(spotlightSweepPhase * DEG_TO_RAD), 0, 0, 1); // Swing side to side
	glLightfv(GL_LIGHT1, GL_POSITION, spotPosition);
	glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, spotDirection);
	glPopMatrix();
}

/*
	Combine 4 little-endian bytes (lowest byte first) into one int.
	Each later byte is worth 256 times more, so it's shifted 8 bits further left.
*/
int readInt32(const unsigned char* bytes) {
	return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

/*
	Load a 24-bit BMP file. Returns tightly packed RGB pixels, bottom row first
	(the order OpenGL expects), or NULL if it can't be loaded. The caller must free() the result.
*/
unsigned char* loadBMP(const char* filename, int* width, int* height) {
	unsigned char header[54];
	FILE* file = fopen(filename, "rb");   // "b" = binary mode, so Windows doesn't alter any bytes
	if (file == NULL) {
		return NULL;
	}

	// Read the 54-byte header and check it starts with "BM".
	if (fread(header, 1, 54, file) != 54 || header[0] != 'B' || header[1] != 'M') {
		fclose(file);
		return NULL;
	}

	int dataOffset = readInt32(header + 10);
	*width = readInt32(header + 18);
	*height = readInt32(header + 22);
	int bitsPerPixel = header[28] | (header[29] << 8);

	if (bitsPerPixel != 24 || *width <= 0 || *height <= 0) {
		fclose(file);
		return NULL;
	}

	// 3 bytes per pixel, and each row in the file is padded up to a multiple of 4 bytes.
	int rowSize = *width * 3;
	int padding = (4 - rowSize % 4) % 4;

	unsigned char* pixels = (unsigned char*)malloc(rowSize * *height);
	if (pixels == NULL) {
		fclose(file);
		return NULL;
	}

	// Jump to the pixel data, then read one row at a time, skipping the padding after each.
	fseek(file, dataOffset, SEEK_SET);
	for (int row = 0; row < *height; row++) {
		unsigned char* rowStart = pixels + row * rowSize;
		if (fread(rowStart, 1, rowSize, file) != rowSize) {
			free(pixels);
			fclose(file);
			return NULL;
		}
		fseek(file, padding, SEEK_CUR);

		// BMP stores B, G, R: swap the first and third byte of each pixel to get R, G, B.
		for (int i = 0; i < rowSize; i += 3) {
			unsigned char blue = rowStart[i];
			rowStart[i] = rowStart[i + 2];
			rowStart[i + 2] = blue;
		}
	}

	fclose(file);
	return pixels;
}

/*
	Load a BMP and upload it to OpenGL. Returns the texture ID, or 0 if loading failed.
	Only call from init(): loading every frame would be very slow.
*/
GLuint loadTexture(const char* filename) {
	int width, height;
	unsigned char* pixels = loadBMP(filename, &width, &height);
	if (pixels == NULL) {
		printf("Couldn't load texture %s\n", filename);
		return 0;
	}

	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	// Repeat the image when texture coordinates go past 1.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// The default min filter expects mipmaps, which we don't make, so the texture would show white.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Our rows have no padding, so tell OpenGL not to expect each row to start on a 4-byte boundary.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	// OpenGL has its own copy now.
	free(pixels);
	return textureId;
}

/*
	Seabed height in metres at a world position. Three waves of different sizes, running in
	different directions, are added together so the hills look irregular. The same spot always
	gives the same height, so the hills stay put while the ground follows the octopus.
*/
float groundHeight(float x, float z) {
	if (!hillsEnabled) {
		return 0.0f;   // Flat seabed
	}
	return 1.5f * sinf(0.06f * x + 0.03f * z)
		+ 1.0f * sinf(0.09f * z - 0.05f * x + 1.0f)
		+ 0.5f * sinf(0.21f * x + 0.17f * z + 2.0f);
}

/*
	One seabed corner: a normal tilted to match the slope, a world-based texture coordinate,
	and the vertex raised to groundHeight().
*/
void drawGroundVertex(float x, float z) {
	// Slope: compare the heights 1 m either side in x and in z.
	float slopeX = groundHeight(x + 1.0f, z) - groundHeight(x - 1.0f, z);
	float slopeZ = groundHeight(x, z + 1.0f) - groundHeight(x, z - 1.0f);
	glNormal3f(-slopeX, 2.0f, -slopeZ);

	// The image repeats every SEABED_TEXTURE_SIZE metres.
	glTexCoord2f(x / SEABED_TEXTURE_SIZE, z / SEABED_TEXTURE_SIZE);
	glVertex3f(x, groundHeight(x, z), z);
}

/*
	Draw a square of seabed tiles centred on (centreX, centreZ), skipping a hole of holeSize
	in the middle (0 for no hole).
*/
void drawGroundSquare(int centreX, int centreZ, int size, int tileSize, int holeSize) {
	for (int x = centreX - size / 2; x < centreX + size / 2; x += tileSize) {
		for (int z = centreZ - size / 2; z < centreZ + size / 2; z += tileSize) {
			// Skip tiles inside the hole, where the near ground is drawn instead.
			if (x >= centreX - holeSize / 2 && x < centreX + holeSize / 2 &&
				z >= centreZ - holeSize / 2 && z < centreZ + holeSize / 2) {
				continue;
			}

			glBegin(GL_QUADS);
			drawGroundVertex(x, z);
			drawGroundVertex(x, z + tileSize);
			drawGroundVertex(x + tileSize, z + tileSize);
			drawGroundVertex(x + tileSize, z);
			glEnd();
		}
	}
}

/****************************************2026****************************************/