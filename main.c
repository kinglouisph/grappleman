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
#include "drawing.h"

#define GLT_IMPLEMENTATION
#define GLT_MANUAL_VIEWPORT
#include "glText.h"

//opengl
uint32_t VBO;
uint32_t VAO;
uint32_t EBO;
unsigned int shaderProgram;

//camera data
uint32_t windowWidth=700;
uint32_t windowHeight=700;
float zoom = 1.0f;
float maxZoom = 4.0f;
float minZoom = 0.25f;

//mouse data
float mx=0.001f;
float my=0.001f;
char mdown = 0;
char mclick = 0;
char m2down = 0;
char m2click = 0;

float grapplex;
float grappley;
float grapplevx;
float grapplevy;
float grappling;
float grappleHooked;
float grappleLen;
float grappleSpeed = 0.085f;
float grappleSpringConst = 0.005f;


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
		mdown = 1;
		mclick = 1;
	}
	if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		m2down = 1;
		m2click = 1;
	}
	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		mdown = 0;
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
	
	float drawing[8];
	
} Platform;

//player position and velocity
float px, py, pvx, pvy;


int main() {
	srand (time(NULL));
	
	int windowWidth = 700;
	int windowHeight = 700;
	
	
	const char* vertexShaderSrc = "#version 330 core\n"
		"uniform float zoom;"
		"uniform vec2 camPos;"
		"layout (location=0) in vec2 triPos;"
		"void main(){"
		"vec2 npos=zoom * (triPos - camPos);"
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
	gltSetText(instructionText, "Instructions:\n");



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
	char inMenu = 1;
	
	float mapWidth = 75.0f;
	float mapHeight = 70.0f;
	int numPlatforms = 1800;
	float platformWidthMul = 0.2f;
	float platformWidthMin = 0.06f;
	float platformHeightMul = 0.04f;
	float platformHeightMin = 0.04f;
	int xchunks = 5;
	int ychunks = 5;
	
	float maxDist = (sqr(platformWidthMin + platformWidthMul) + sqr(platformHeightMin + platformHeightMul));
	
	Platform* platforms = malloc(sizeof(Platform) * numPlatforms);
	
	char platformDraw[] = {
		0,1,2,
		1,2,3
	};
	
	
	double fpsclastTime = glfwGetTime();
	int fpscnbFrames = 0;
	
	int trailCount = 1500;
	
	//test for OpenGL errors
	{int err = glGetError();if (err) {printf(getGLErrorStr(err));
				printf("b\n");fflush(stdout);}}
	
	float trails[trailCount * 2];
	int trailsNum;
	
	float lastpx, lastpy;
	
	
	
	while (!glfwWindowShouldClose(window)) {
		double oldTime = glfwGetTime();
		
		mclick = 0;
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
		
    float mouseDist = sqrt(mx*mx+my*my);
    float t1=mx / mouseDist;
    float t2=my / mouseDist;
         
    float angle = atan2(t2,t1);
		
		
		if (inMenu) {
			if (mclick && mx > -0.7f && mx < 0.7f && my > -0.1f && my < 0.1f) {
				//initialize game
				inMenu = 0;
				px = mapWidth - 1.0f;
				py = mapHeight/2;
				pvx = 0;
				pvy = 0;
				
				grapplex = 0;
				grappley = 0;
				grapplevx = 0;
				grapplevy = 0;
				grappling = 0;
				grappleHooked = 0;

				zoom = 0.5f;
				glUseProgram(shaderProgram);
				glUniform1f(zoomLocation, zoom);
				
				//generate map
				for (int i = 0; i < numPlatforms; i++) {
					float r1 = randf() * mapWidth;
					float r2 = randf() * mapHeight;
					float r3 = randf() * platformWidthMul + platformWidthMin;
					float r4 = randf() * platformHeightMul + platformHeightMin;
					
					platforms[i].x1 = r1 - r3*0.5f;
					platforms[i].y1 = r2 - r4*0.5f;
					platforms[i].x2 = r1 + r3*0.5f;
					platforms[i].y2 = r2 + r4*0.5f;
					
					platforms[i].drawing[0]=platforms[i].x1;
					platforms[i].drawing[1]=platforms[i].y1;
					platforms[i].drawing[2]=platforms[i].x2;
					platforms[i].drawing[3]=platforms[i].y1;
					platforms[i].drawing[4]=platforms[i].x1;
					platforms[i].drawing[5]=platforms[i].y2;
					platforms[i].drawing[6]=platforms[i].x2;
					platforms[i].drawing[7]=platforms[i].y2;
				}
				
			}
			
			lastpx = px;
			lastpy = py;
			
			trailsNum = 0;
			for (int i = 0; i < trailCount*2; i+=2) {
				trails[i] = px;
				trails[i+1] = py;
			}
		} else {
			//gravity
			pvy -= 0.00012f;
			
			//game logic
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
			
			if (grappling) {
				grapplex += grapplevx;
				grappley += grapplevy;
				
				if (!grappleHooked) {
					for (int i = 0; i < numPlatforms; i++) {
						char brk = 0;
						if (abs(grapplex - platforms[i].x1) < 6.0f && abs(grappley - platforms[i].y1) < 6.0f) {
							grapplex -= grapplevx;
							grappley -= grapplevy;
							for (int j = 0; j < 5; j++) {
								grapplex += grapplevx*0.2f;
								grappley += grapplevy*0.2f;
								if (grapplex > platforms[i].x1 && grapplex < platforms[i].x2 && grappley > platforms[i].y1 && grappley < platforms[i].y2) {
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
			
			float v = sqrt(pvx*pvx+pvy*pvy);
			
			pvx -= pvx * 0.02f * v;
			pvy -= pvy * 0.02f * v;
			
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
			glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			glUniform4f(colorLocation, .3f,.3f,.3f,1.0f);
			{int err = glGetError();if (err) {printf(getGLErrorStr(err));
				printf("d\n");fflush(stdout);}}
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 8, menuVerts);
			{int err = glGetError();if (err) {printf(getGLErrorStr(err));
				printf("e\n");fflush(stdout);}}
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, 6, menuInds);
			{int err = glGetError();if (err) {printf(getGLErrorStr(err));
				printf("f\n");fflush(stdout);}}
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
			glUseProgram(0);
			
			gltBeginDraw();
			gltColor(1.0f,1.0f,1.0f,1.0f);
			gltDrawText2DAligned(titleText,(float)windowWidth/2,0.2f*(float)windowHeight/2,6.0f, GLT_CENTER, GLT_CENTER);
			gltDrawText2DAligned(startText,(float)windowWidth/2,0.5f*(float)windowHeight,4.0f, GLT_CENTER, GLT_CENTER);
			
			gltEndDraw();
		} else {
			glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			//draw platforms
			glUniform2f(camPosLocation, px, py);
			glUniform4f(colorLocation, 0.95f, 0.95f, 0.95f, 1.0f);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, 6, platformDraw);
			for (int i = 0; i < numPlatforms; i++) {
				float camx = px - platforms[i].x1;
				float camy = py - platforms[i].y1;
				
				
				if (abs(camy) > 10.0f) {
					continue;
				}
				if (abs(camx) > 10.0f) {
					camx -= mapWidth;
					if (abs(camx) > 10.0f) {
						camx += 2*mapWidth;
						if (abs(camx) > 10.0f) {
							continue;
						}
					}
				}
				
				
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 8, platforms[i].drawing);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
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