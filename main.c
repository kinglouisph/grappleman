#ifdef WIN32
void* __chk_fail=0; //weord fix to compile for windows
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>

#define GLT_IMPLEMENTATION
#define GLT_MANUAL_VIEWPORT
#include "glText.h"
#include "drawing.h"


//opengl
uint32_t VBO;
uint32_t VAO;
uint32_t EBO;
unsigned int shaderProgram;

//camera data
uint32_t windowWidth=700;
uint32_t windowHeight=700;
float zoom = 1.0f;
float maxZoom = 8.0f;
float minZoom = 0.25f;

//mouse data
float mx=0.001f;
float my=0.001f;
char m1down = 0;
char m1click = 0;
char m2down = 0;
char m2click = 0;

float grapplex;
float grappley;
float grapplevx;
float grapplevy;
float grappling;
float grappleHooked;
float grappleLen;
float grappleSpeed = 0.095f;
float grappleSpringConst = 0.005f;
float maxGrappleLenSquared = 22.8 * 22.8f;

int weapon;
char plDead;


float sqr(float a) {
	return a*a;
}

const char* getGLErrorStr(GLenum err) {
	switch (err) {
		case GL_NO_ERROR:          return "No error";
		case GL_INVALID_ENUM:      return "Invalid enum";
		case GL_INVALID_VALUE:     return "Invalid value";
		case GL_INVALID_OPERATION: return "Invalid operation";
					   //case GL_STACK_OVERFLOW:    return "Stack overflow";
					   //case GL_STACK_UNDERFLOW:   return "Stack underflow";
		case GL_OUT_OF_MEMORY:     return "Out of memory";
		default:                   return "Unknown error";
	}
}

float randf() {
	return (float)rand() / (float)RAND_MAX;
}


float modulof(float a, float b) {
	return a - b * (float)floor(a / b);
}
void mouse_callback(GLFWwindow* window, double dmposx, double dmposy) {
	mx = (float)(dmposx - windowWidth/2) / (float)(windowWidth/2);
	my = -1.0f*(float)(dmposy - windowHeight/2) / (float)(windowHeight/2);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		m1down = 1;
		m1click = 1;
	}
	if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		m2down = 1;
		m2click = 1;
	}
	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		m1down = 0;
	}
	if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		m2down = 0;
	}
}

typedef struct Platform {
	float x1;//top left
	float y1;
	float x2;//bottom right
	float y2;
	
	void* next;
	
	float drawing[8];
	
} Platform;

typedef struct Swarm {
	float color1;
	float color2;
	float color3;
	float x;
	float y;
	float vx;
	float vy;
	float ax;
	float ay;
	float* model;
	float angle;
	float size;
	
	char hasWeapon;
	char hp;
	
	float turnspeed;
	float speed;
	struct Swarm* next;
	struct Swarm* prev;
	
} Swarm;

typedef struct Projectile {
	float x;
	float y;
	float vx;
	float vy;
	float collisionRadius;
	float angle;
	struct Projectile* next;
	struct Projectile* prev;
} Projectile;

Projectile* firstProjectile;
Projectile* lastProjectile;
//player position and velocity
float px, py, pvx, pvy;

//map data
float mapWidth = 75.0f;
float mapHeight = 70.0f;
int numPlatforms = 1800;
float platformWidthMul = 0.35f;
float platformWidthMin = 0.1f;
float platformHeightMul = 0.04f;
float platformHeightMin = 0.02f;
#define xchunks 7
#define ychunks 7
#define pxchunk 3
#define pychunk 3
int platformsPerChunkMin = 3;
float platformsPerChunkRand = 7.0f;
float chunkSize = 4.0f;

long pchunkx = 0;
long pchunky = 0;
	
Platform* platforms[xchunks][ychunks];


void genPlatforms(int x, int y, int x2, int y2) {
	Platform** platPtr = &platforms[x][y];
						
	int numPlatforms = platformsPerChunkMin + floor(randf() * platformsPerChunkRand);
	
	for (int i = 0; i < numPlatforms; i++) {
		Platform* platform = malloc(sizeof(Platform));
		if (platform == NULL) {
			printf("Malloc fail\n");
			fflush(stdout);
			exit(1);
		}
		platform->next = NULL;
		*platPtr = platform;
		platPtr = (Platform**) &platform->next;
		
		float r1 = randf() * chunkSize;
		float r2 = randf() * chunkSize;
		float r3 = randf() * platformWidthMul + platformWidthMin;
		float r4 = randf() * platformHeightMul + platformHeightMin;
		
		platform->x1 = r1 - r3*0.5f + (float)(x2 + x) * chunkSize;
		platform->y1 = r2 - r4*0.5f + (float)(y2 + y) * chunkSize;
		platform->x2 = r1 + r3*0.5f + (float)(x2 + x) * chunkSize;
		platform->y2 = r2 + r4*0.5f + (float)(y2 + y) * chunkSize;
		
		platform->drawing[0]= platform->x1;
		platform->drawing[1]= platform->y1;
		platform->drawing[2]= platform->x2;
		platform->drawing[3]= platform->y1;
		platform->drawing[4]= platform->x1;
		platform->drawing[5]= platform->y2;
		platform->drawing[6]= platform->x2;
		platform->drawing[7]= platform->y2;
	}
}

int enemyTimer;
int maxEnemyTimer;

Swarm* firstSwarm = NULL;
Swarm* lastSwarm = NULL;

char hasDied = 0;
char inMenu = 1;

void killPlayer() {
	plDead = 1;
	inMenu = 1;
	hasDied = 1;
	Swarm* s = firstSwarm;
	while (s != NULL) {
		Swarm* n = s->next;
		free(s);
		s = n;
	}
	firstSwarm = NULL;
	lastSwarm = NULL;
	
	for (int i = 0; i < xchunks; i++) {
		for (int j = 0; j < ychunks; j++) {
			Platform* platform = platforms[i][j];
			
			while (1) {
				Platform* n = platform->next;
				free(platform);
				platform = n;
				if (platform == NULL) {
					break;
				}
			}
			
			platforms[i][j] = NULL;
		}
	}
}


void killSwarm(Swarm* s) {
	if (s == firstSwarm && s == lastSwarm) {
		free(s);
		firstSwarm = NULL;
		lastSwarm = NULL;
	} else if (s == firstSwarm) {
		firstSwarm = s->next;
		s->next->prev = NULL;
		free(s);
	} else if (s == lastSwarm) {
		lastSwarm = s->prev;
		s->prev->next = NULL;
		free(s);
	} else {
		s->prev->next = s->next;
		s->next->prev = s->prev;
		free(s);
	}
}

void killProjectile(Projectile* s) {
	if (s == firstProjectile && s == lastProjectile) {
		free(s);
		firstProjectile = NULL;
		lastProjectile = NULL;
	} else if (s == firstProjectile) {
		firstProjectile = s->next;
		s->next->prev = NULL;
		free(s);
	} else if (s == lastProjectile) {
		lastProjectile = s->prev;
		s->prev->next = NULL;
		free(s);
	} else {
		s->prev->next = s->next;
		s->next->prev = s->prev;
		free(s);
	}
}



int main() {
	srand (time(NULL));
	
	int windowWidth = 700;
	int windowHeight = 700;
	
	
	const char* vertexShaderSrc = "#version 330 core\n"
		"uniform float zoom;"
		"uniform float angle;"
		"uniform vec2 camPos;"
		"uniform vec2 center;" //for rotation
		"layout (location=0) in vec2 triPos;"
		"void main(){"
		"vec2 tpos = triPos - center;"
		"vec2 npos = vec2(tpos.x*cos(angle) - tpos.y*sin(angle),tpos.x*sin(angle) + tpos.y*cos(angle)) + center;"
		"npos=zoom * (npos - camPos);"
		"gl_Position=vec4(npos,0,1);}\0";

	const char* fragmentShaderSrc = "#version 330 core\n"
		"out vec4 outColor;"
		"uniform vec4 color;"
		"void main(){outColor=vec4(color);}\0";

	#pragma region

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
			
	GLFWwindow* window = glfwCreateWindow(windowWidth,windowHeight,"Grappleman",NULL,NULL);
	if(window==NULL){
		printf("no window :(");
		glfwTerminate();
		return 1;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
		printf("Failed to initialize GLAD\n");
		return -1;
	}   

	//capture mouse
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 
	glfwSetCursorPosCallback(window, mouse_callback);  
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glViewport(0,0,windowWidth,windowHeight);


	//shaders
	int success;
	char infoLog[512];
	uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader,1,&vertexShaderSrc,NULL);
	glCompileShader(vertexShader);

	glGetShaderiv(vertexShader,GL_COMPILE_STATUS,&success);
	if(!success){
		glGetShaderInfoLog(vertexShader,512,NULL,infoLog);
		printf(infoLog);
		return 1;
	}

	uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader,1,&fragmentShaderSrc,NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader,GL_COMPILE_STATUS,&success);
	if(!success){
		glGetShaderInfoLog(fragmentShader,512,NULL,infoLog);
		printf(infoLog);
		return 1;
	}

	shaderProgram=glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glGetProgramiv(shaderProgram,GL_LINK_STATUS,&success);


	if(!success){
		glGetProgramInfoLog(shaderProgram,512,NULL,infoLog);
		printf(infoLog);
		return 1;
	}


	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);


	//text library
	if (!gltInit()){
		fprintf(stderr, "Failed to initialize glText\n");
		fflush(stdout);
		return 1;
	}
	
	gltViewport(windowWidth, windowHeight);
	
	GLTtext* titleText = gltCreateText();
	gltSetText(titleText, "Grappleman");
	GLTtext* startText = gltCreateText();
	gltSetText(startText, "Start");
	GLTtext* instructionText = gltCreateText();
	gltSetText(instructionText, "Instructions:\n Left Click: Fire\nRight Click: Grapple, hold to reel in\n QE: Control zoom");
	GLTtext* deadText = gltCreateText();
	gltSetText(deadText, "You Died");
	GLTtext* scoreText = gltCreateText();
	gltSetText(scoreText, "Final Score: 0");
	GLTtext* scoreNumber = gltCreateText();
	gltSetText(scoreNumber, "0");


	glUseProgram(shaderProgram);

	glGenVertexArrays(1,&VAO);
	glGenBuffers(1,&VBO);
	glGenBuffers(1,&EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, 4096, NULL, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 1024, NULL, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
	glEnableVertexAttribArray(0);
	
	//more graphics stuff
	const int camPosLocation = glGetUniformLocation(shaderProgram, "camPos");
	const int colorLocation = glGetUniformLocation(shaderProgram, "color");
	const int zoomLocation = glGetUniformLocation(shaderProgram, "zoom");
	const int centerLocation = glGetUniformLocation(shaderProgram, "center");
	const int angleLocation = glGetUniformLocation(shaderProgram, "angle");
	
	
	//float invWindowWidth = 1.0f / (float)windowWidth;
	//float invWindowHeight = 1.0f / (float)windowHeight;
	/*float projection[] = {
		1.000000, 0.000000, 0.000000, 0.000000,
		0.000000, 1.000000, 0.000000, 0.000000,
		0.000000, 0.000000, -1.00000, 0.000000,
		0.000000, 0.000000, 0.000000, 1.000000
	};*/
	
	#pragma endregion

	//glUniformMatrix4fv(projectionLocation, 1, 0, projection);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
	//glfwSwapInterval(0);
	
	glUniform1f(zoomLocation, 1.0f);
	glUniform1f(angleLocation, 0.0f);
	glUniform2f(centerLocation, 0.0f, 0.0f);
	
	//setup drawing models
	
	//triangle fan
	int playerModelSize = 62;
	float playerModel[playerModelSize];
	float playerSize = 0.02f; //radius
	
	{
		playerModel[0]=0;
		playerModel[1]=0;
		float a = 0.0f;
		float b = M_PI * 4.0f / (float)(playerModelSize-4);
		for (int i = 2; i < playerModelSize; i+=2) {
			playerModel[i] = cos(a) * playerSize;
			playerModel[i+1] = sin(a) * playerSize;
			a+=b;
		}
	}
	
	
	
	//some game data
	
	
	
	//Platform* platforms = malloc(sizeof(Platform) * numPlatforms);
	
	
	
	char platformDraw[] = {
		0,1,2,
		1,2,3
	};
	
	//mx+b, squared radius
	char circleLineCollision(float m, float b, float r2) {
		return m*m*b*b - (m*m+1)*(b*b-r2) > 0;
	}
	
	double fpsclastTime = glfwGetTime();
	int fpscnbFrames = 0;
	
	int trailCount = 100;
	
	//test for OpenGL errors
	{int err = glGetError();if (err) {printf(getGLErrorStr(err));
				printf("b\n");fflush(stdout);}}
	
	float trails[trailCount * 2];
	int trailsNum;
	
	float lastpx, lastpy;
	
	int enemySpawns;
	int score;
	
	while (!glfwWindowShouldClose(window)) {
		double oldTime = glfwGetTime();
		
		m1click = 0;
		m2click = 0;
		glfwPollEvents();
		
		
		
		//fps counter
		double fpsccurrentTime = glfwGetTime();
		fpscnbFrames++;
		if ( fpsccurrentTime - fpsclastTime >= 1.0 ){ // If last prinf() was more than 1 sec ago
		    // printf and reset timer
		    printf("%d fps\n", fpscnbFrames);
		    fpscnbFrames = 0;
		    fpsclastTime += 1.0;
		}
         
    float angle = atan2(my,mx);
		
		float altitudeStart = 25.0f;
		float spaceStart = 85.0f;
		
		float altitudeValue = (abs(py - altitudeStart - spaceStart) - abs(py - altitudeStart)) / (2.0f * spaceStart) + 0.5f;
		
		
		
		if (inMenu) {
			if (m1click && mx > -0.7f && mx < 0.7f && my > -0.1f && my < 0.1f) {
				//initialize game
				inMenu = 0;
				score = 0;
				gltSetText(scoreNumber, "0");
				
				px = 0;
				py = 0;
				pvx = 0;
				pvy = 0;
				pchunkx = 0;
				pchunky = 0;
				
				grapplex = 0;
				grappley = 0;
				grapplevx = 0;
				grapplevy = 0;
				grappling = 0;
				grappleHooked = 0;
				weapon = 0;
				plDead = 0;
				

				zoom = 0.4f;
				glUseProgram(shaderProgram);
				glUniform1f(zoomLocation, zoom);
				
				//generate map
				for (int x = 0; x < xchunks; x++) {
					for (int y = 0; y < ychunks; y++) {
						genPlatforms(x, y, -pychunk, -pychunk);
					}
				}
				
				//printf("got here\n");
				
				lastpx = px;
				lastpy = py;
				
				trailsNum = 0;
				for (int i = 0; i < trailCount*2; i+=2) {
					trails[i] = px;
					trails[i+1] = py;
				}
				enemyTimer = 590;
				maxEnemyTimer = 600;
				enemySpawns = 0;
			
			}
		} else {
			//game logic
			
			//gravity
			pvy -= 0.0001f * altitudeValue;
			
			//grapple send
			if (m2click) {
				if (grappling) {
					grappling = 0;
					grappleHooked = 0;
				} else {
					grappling = 1;
					grapplex = px;
					grappley = py;
					grapplevx = grappleSpeed * cos(angle);
					grapplevy = grappleSpeed * sin(angle);
					grappleHooked = 0;
				}
			}
			
			//attack
			if (m1click) {
				/*switch (weapon) {
					case 0:
					//triangle melee
					
					//attack is a triangle
					const float meleeRange = 0.1f;
					const float meleeBase = 0.04f; //radius
					
					
					break;
				}*/
				
				Projectile* p = malloc(sizeof(Projectile));
				p->x = px;
				p->y = py;
				p->vx = 0.05f * cos(angle) + pvx;
				p->vy = 0.05f * sin(angle) + pvy;
				p->angle = angle;
				p->next = NULL;
				p->prev = NULL;
				if (firstProjectile == NULL) {
					firstProjectile = p;
					lastProjectile = p;
				} else {
					p->prev = lastProjectile;
					lastProjectile->next = p;
					lastProjectile = p;
				}
			}
			
			
			//set chunks
			long npchunkx = floorl(px / chunkSize);
			long npchunky = floorl(py / chunkSize);
			
			int deltachunkx = pchunkx - npchunkx;
			int deltachunky = pchunky - npchunky;
			
			pchunkx = npchunkx;
			pchunky = npchunky;
			
			
			
			
			
			while (deltachunkx > 0) {//player went left
				deltachunkx--;
				
				for (int i = 0; i < ychunks; i++) {
					Platform* platform = platforms[xchunks-1][i];
					
					while (1) {
						Platform* n = platform->next;
						free(platform);
						platform = n;
						if (platform == NULL) {
							break;
						}
					}
				}
				
				for (int i = xchunks-2; i > -1; i--) {
					for (int j = 0; j < ychunks; j++) {
						platforms[i+1][j] = platforms[i][j];
					}
				}
				
				for (int i = 0; i < ychunks; i++) {
					genPlatforms(0, i, npchunkx - pxchunk, npchunky-pychunk);
				}
			}
			
			while (deltachunkx < 0) {//player went right
				deltachunkx++;
				
				for (int i = 0; i < ychunks; i++) {
					Platform* platform = platforms[0][i];
					
					while (1) {
						Platform* n = platform->next;
						free(platform);
						platform = n;
						if (platform == NULL) {
							break;
						}
					}
				}
				
				for (int i = 0; i < xchunks-1; i++) {
					for (int j = 0; j < ychunks; j++) {
						platforms[i][j] = platforms[i+1][j];
					}
				}
				
				for (int i = 0; i < ychunks; i++) {
					genPlatforms(xchunks-1, i, npchunkx-pxchunk, npchunky-pychunk);
				}
			}
			
			
			
			while (deltachunky > 0) {//player went down
				deltachunky--;
				
				for (int i = 0; i < xchunks; i++) {
					Platform* platform = platforms[i][ychunks-1];
					
					while (1) {
						Platform* n = platform->next;
						free(platform);
						platform = n;
						if (platform == NULL) {
							break;
						}
					}
				}
				
				for (int i = 0; i < xchunks; i++) {
					for (int j = ychunks-2; j > -1; j--) {
						platforms[i][j+1] = platforms[i][j];
					}
				}
				
				for (int i = 0; i < xchunks; i++) {
					genPlatforms(i, 0, npchunkx-pxchunk, npchunky-pychunk);
				}
			}
			
			
			while (deltachunky < 0) {//player went up
				deltachunky++;
				
				for (int i = 0; i < xchunks; i++) {
					Platform* platform = platforms[i][0];
					
					while (1) {
						Platform* n = platform->next;
						free(platform);
						platform = n;
						if (platform == NULL) {
							break;
						}
					}
				}
				
				for (int i = 0; i < xchunks; i++) {
					for (int j = 0; j < ychunks - 1; j++) {
						platforms[i][j] = platforms[i][j+1];
					}
				}
				
				for (int i = 0; i < xchunks; i++) {
					genPlatforms(i, ychunks-1, npchunkx-pxchunk, npchunky-pychunk);
				}
			}
			
			
			//spawn enemies
			if (enemyTimer >= maxEnemyTimer) {
				enemyTimer = 0;
				
				int spawningPoints = enemySpawns * 5 + 5 + floor(randf() * 10);
				enemySpawns++;
				
				float rand2 = randf()*0.35f;
				float rand3 = randf()*0.35f;
				
				float angle = randf() * M_PI * 2.0f;
				Swarm* prev = lastSwarm;
				int i = 0;
				while (i < spawningPoints) {
					float rand = randf(); // enemy type
					
					Swarm* a = malloc(sizeof(Swarm));
					a->x = cos(angle) * 5.0f + 0.4f * randf()-0.2f + px;
					a->y = sin(angle) * 5.0f + 0.4f * randf()-0.2f + py;
					a->vx = 0;
					a->vy = 0;
					a->angle = 0;
					a->size = 0.05f;
					if (rand < 0.05f + rand2 && enemySpawns > 3) {
						//green swarm
						a->color1 = 0.45f;
						a->color2 = 0.99f;
						a->color3 = 0.45f;
						
						a->hasWeapon = 0;
						a->hp = 2;
						
						a->speed = 1.2f;
						i+=2;
					} else if (rand > 0.95f -rand3 && enemySpawns > 2) {
						//blue (ranged) swarm
						a->color1 = 0.62f;
						a->color2 = 0.75f;
						a->color3 = 0.99f;
						
						a->hasWeapon = 1;
						a->hp = 2;
						
						a->speed = 1.0f;
						i+=3;
					} else  {
						//red swarm
						a->color1 = 0.95f;
						a->color2 = 0.35f;
						a->color3 = 0.12f;
						
						a->hasWeapon = 0;
						a->hp = 1;
						
						a->speed = 1.0f;
						i++;
					}
					a->next = NULL;
					a->prev = prev;
					if (a->prev) {
						a->prev->next = a;
					} else {
						firstSwarm = a;
					}
					prev = a;
					
					lastSwarm = a;
				
					
				}
			}
			enemyTimer++;
			
			int swarmCount = 0;
			//swarm logic
			for (Swarm* enemy = firstSwarm; enemy != NULL; enemy = enemy->next) {	
				enemy->ax=(px-enemy->x);
				enemy->ay=(py-enemy->y);
				if (sqr(enemy->ax) + sqr(enemy->ay) > 400.0f) {//despawn after 20 dist
					if (enemy->prev != NULL) {//the first spawned enemy won't despawn. oh well.
						if (enemy->next == NULL)
							lastSwarm = enemy->prev; 
						else
							enemy->next->prev = enemy->prev;
						
						enemy->prev->next = enemy->next;
						Swarm* t = enemy->prev;
						free(enemy);
						enemy=t;
						continue;
					}
				}
				
				if (sqr(enemy->ax) + sqr(enemy->ay) < sqr(enemy->size + playerSize-.01f)) {
					plDead = 1;
				}
				
				swarmCount++;
				 
			}
			{
				int i = 0;
				for (Swarm* enemy1 = firstSwarm; i < swarmCount - 1; enemy1 = enemy1->next) {
					int j = i+1;
					for (Swarm* enemy2 = enemy1->next; j < swarmCount; enemy2 = enemy2->next) {
						float dx = enemy1->x - enemy2->x;
						float dy = enemy1->y - enemy2->y;
						float d = dx*dx+dy*dy;
						dx /= d;
						dy /= d;
						
						enemy1->ax += dx/d * 0.00012;
						enemy1->ay += dy/d * 0.00012;
						enemy2->ax -= dx/d * 0.00012;
						enemy2->ay -= dy/d * 0.00012;
						
					
					
						j++;
					}
					i++;
				}
			}
			for (Swarm* enemy = firstSwarm; enemy != NULL; enemy = enemy->next) {
				
				float mag = sqrt(enemy->ax*enemy->ax + enemy->ay*enemy->ay);
				float invmag = 0.0005f/mag;
				enemy->ax*=invmag;
				enemy->ay*=invmag;
				enemy->vx += enemy->ax * enemy->speed;
				enemy->vy += enemy->ay * enemy->speed;
				enemy->vx *= .993;
				enemy->vy *= .993;
				enemy->x += enemy->vx;
				enemy->y += enemy->vy;
				enemy->angle = atan2(enemy->ay, enemy->ax);
				 
			}
			
			//projectile logic
			Projectile* p = firstProjectile;
			while (p!=NULL) {
				p->x+=p->vx;
				p->y+=p->vy;
				char alive = 1;
				
				//despawn if far enough
				if (sqr(p->x-px) + sqr(p->y-py) > 400.0f) {
					Projectile* t = p;
					p = p->next;
					killProjectile(t);
					continue;
				}
				
				Swarm* e = firstSwarm;
				while (e != NULL) {
					float d2 = sqr(e->x-p->x) + sqr(e->y-p->y);
					if (d2 < e->size * e->size) {
						Projectile* t = p;
						p = p->next;
						killProjectile(t);
						e->hp-=1;
						if (e->hp < 1) {
							killSwarm(e);
						}
						score++;
						alive = 0;
						
						char txt[16];
						sprintf(txt, "%d", score);
						gltSetText(scoreNumber, txt);
						break;
					} else {
						e = e->next;
					}
				}
				
				if (alive){ 
					p=p->next;
				}
			}
			
			
			
			
			
			if (grappling) {
				grapplex += grapplevx;
				grappley += grapplevy;
				
				if (!grappleHooked && (sqr(px-grapplex) + sqr(py-grappley)) > maxGrappleLenSquared) {
					grappling = 0;
				} else {
					if (!grappleHooked) {
						
						
						char brk = 0;
						
						for (int xi = 0; xi < 2; xi++) {
							for (int yi = 0; yi < 2; yi++) {
								int x = xi - 1 + floor(grapplex / chunkSize) - floor(px / chunkSize) + pxchunk;
								int y = yi - 1 + floor(grappley / chunkSize) - floor(py / chunkSize) + pychunk;
								
								if (x < 0 || y < 0 || x > xchunks-1 || y > ychunks-1) {
									continue;
								}
								
								Platform* platform = platforms[x][y];
								
								while (1) {
									if (abs(grapplex - platform->x1) < 6.0f && abs(grappley - platform->y1) < 6.0f) {
										grapplex -= grapplevx;
										grappley -= grapplevy;
										for (int j = 0; j < 5; j++) {
											grapplex += grapplevx*0.2f;
											grappley += grapplevy*0.2f;
											if (grapplex > platform->x1 && grapplex < platform->x2 && grappley > platform->y1 && grappley < platform->y2) {
												grappleHooked = 1;
												grappleLen = sqrt(sqr(px-grapplex)+sqr(py-grappley));
												grapplevx=0;
												grapplevy=0;
												
												brk = 1;
												break;
												
											}
										}
										
										if (brk) {break;}
									}
									
									platform = platform->next;
									if (platform == NULL) {
										break;
									}
								}
								if (brk) {break;}
							}
							if (brk) {break;}
						}
						
					}
					if (grappleHooked) {
						float actualLen = sqrt(sqr(px-grapplex)+sqr(py-grappley));
						
						if (m2down) {
							if (grappleLen > playerSize) {
								if (grappleLen > actualLen) {
									grappleLen = actualLen;
								}
								grappleLen -= 0.02f;
							}
						}
						
						if (actualLen > grappleLen) {
							float force = grappleSpringConst * (actualLen - grappleLen);
						
							float fx = (grapplex - px) / actualLen;
							float fy = (grappley - py) / actualLen;
							
							pvx += fx * force;
							pvy += fy * force;
						}
					}
				}
			}
			
			float v = sqrt(pvx*pvx+pvy*pvy);
			
			//air resisrance
			pvx -= pvx * 0.01f * v * altitudeValue;
			pvy -= pvy * 0.01f * v * altitudeValue;
			
			px += pvx;
			py += pvy;
		}
		
		
		
		
		
		//render
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
      zoom *= 1.02f;
			if (zoom > maxZoom) {zoom = maxZoom;}
			glUniform1f(zoomLocation, zoom);
    }
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
      zoom /= 1.02f;
			if (zoom < minZoom) {zoom = minZoom;}
			glUniform1f(zoomLocation, zoom);
    }
		
		if (inMenu) {
			zoom = 1.0f;
			glUniform1f(zoomLocation, zoom);
			
			glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			glUniform4f(colorLocation, .3f,.3f,.3f,1.0f);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 8, menuVerts);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, 6, menuInds);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
			glUseProgram(0);
			
			gltBeginDraw();
			gltColor(1.0f,1.0f,1.0f,1.0f);
			gltDrawText2DAligned(titleText,(float)windowWidth/2,0.2f*(float)windowHeight/2,6.0f, GLT_CENTER, GLT_CENTER);
			gltDrawText2DAligned(startText,(float)windowWidth/2,0.5f*(float)windowHeight,4.0f, GLT_CENTER, GLT_CENTER);
			gltDrawText2DAligned(instructionText,(float)windowWidth/2,0.6f*(float)windowHeight,2.0f, GLT_CENTER, GLT_TOP);
			if (hasDied) {
				char txt[30];
				sprintf(txt, "Final Score: %d", score);
				gltSetText(scoreText, txt);
				gltDrawText2DAligned(deadText,(float)windowWidth/2,0.25f*(float)windowHeight,4.0f, GLT_CENTER, GLT_CENTER);
				gltDrawText2DAligned(scoreText,(float)windowWidth/2,0.35f*(float)windowHeight,4.0f, GLT_CENTER, GLT_CENTER);
			}
			
			gltEndDraw();
		} else {
			glClearColor(altitudeValue * 0.05f, altitudeValue * 0.1f, altitudeValue * 0.15f, 1.0f);
    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			//draw platforms
			glUniform2f(camPosLocation, px, py);
			glUniform4f(colorLocation, 0.95f, 0.95f, 0.95f, 1.0f);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, 6, platformDraw);
			
			for (int x=pxchunk-1; x < pxchunk+2; x++) {
				for (int y=pychunk-1; y < pychunk+2; y++) {
					Platform* platform = platforms[x][y];
					while (1) {
						if (platform == NULL) {
							break;
						}
						
						glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 8, platform->drawing);
						glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
						platform = platform->next;
						
						
					}
				}
			}
			
			//grapple
			glUniform2f(camPosLocation, px, py);
			if (grappling) {
				float line[4] = {
					px,py,
					grapplex,
					grappley
				};
				
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 4, line);
				glDrawArrays(GL_LINES, 0, 2);
			}
			
			
			//draw enemies
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 6, swarmVerts);
			glUniform2f(centerLocation, 0, 0);
			for (Swarm* enemy = firstSwarm; enemy != NULL; enemy = enemy->next) {
				glUniform4f(colorLocation, enemy->color1, enemy->color2, enemy->color3, 1.0f);
				glUniform2f(camPosLocation, px-enemy->x, py-enemy->y);
				glUniform1f(angleLocation, enemy->angle);
				
				glDrawArrays(GL_TRIANGLES, 0, 3);
			}
			
			//draw bullets
			glUniform4f(colorLocation, 0.2f, 0.9f, 0.2f, 1.0f);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 6, bulletVerts);
			for (Projectile* bullet = firstProjectile; bullet != NULL; bullet = bullet->next) {
				glUniform2f(camPosLocation, px-bullet->x, py-bullet->y);
				glUniform1f(angleLocation, bullet->angle);
				glDrawArrays(GL_TRIANGLES, 0, 3);
			}
			glUniform1f(angleLocation, 0.0f);
			
			
			
			//trails
			glUniform4f(colorLocation, 0.98f, 0.98f, 0.98f, 0.2f);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * playerModelSize, playerModel);
			
			for (int i = 0; i < trailCount*2; i+=2) {
				glUniform2f(camPosLocation, px-trails[i], py-trails[i+1]);
				glDrawArrays(GL_TRIANGLE_FAN, 0, playerModelSize/2);
			}
			
			
			
			
			
			trails[((trailsNum+1)%trailCount)*2]=px;
			trails[((trailsNum+1)%trailCount)*2+1]=py;
			trails[((trailsNum+2)%trailCount)*2]=px + pvx*0.5f;
			trails[((trailsNum+2)%trailCount)*2+1]=py + pvy*0.5f;
			trailsNum = (trailsNum + 2) % trailCount;
			
			lastpx = px;
			lastpy = py;
			
			//draw player
			glUniform2f(camPosLocation, 0, 0);
			glUniform4f(colorLocation, 0.98f, 0.98f, 0.98f, 1.0f);
			//glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * playerModelSize, playerModel);
			glDrawArrays(GL_TRIANGLE_FAN, 0, playerModelSize/2);
			
			//draw score number
			gltBeginDraw();
			gltDrawText2DAligned(scoreNumber,0.0f,0.0f,4.0f, GLT_LEFT, GLT_TOP);
			gltEndDraw();
			
			//player dead
			if (plDead) {
				killPlayer();
			}
		}
		
		
		
		
		
		
		
		{int err = glGetError();if (err) {printf(getGLErrorStr(err));
				printf("a\n");fflush(stdout);}}
		
		glfwSwapBuffers(window);
		
		double newTime = glfwGetTime();
		
		int sleepTime = 16666 - (int)(1000000.0*(newTime - oldTime));
		
		if (sleepTime > 0) {
			usleep(sleepTime); //60fps
		}
	}
	
}