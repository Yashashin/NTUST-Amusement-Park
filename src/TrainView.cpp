/************************************************************************
     File:        TrainView.cpp

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:     
						The TrainView is the window that actually shows the 
						train. Its a
						GL display canvas (Fl_Gl_Window).  It is held within 
						a TrainWindow
						that is the outer window with all the widgets. 
						The TrainView needs 
						to be aware of the window - since it might need to 
						check the widgets to see how to draw

	  Note:        we need to have pointers to this, but maybe not know 
						about it (beware circular references)

     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#include <iostream>
#include <Fl/fl.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
//#include "GL/gl.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <GL/glu.h>

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"



#ifdef EXAMPLE_SOLUTION
#	include "TrainExample/TrainExample.H"
#endif

#define RED 0
#define ORANGE 1
#define YELLOW 2
#define GREEN 3
#define BLUE 4
#define BLUE2 5
#define PURPLE 6
#define PINK 7
#define DIVIDE_LINE 20.0
//************************************************************************
//
// * Constructor to set up the GL window
//========================================================================
TrainView::
TrainView(int x, int y, int w, int h, const char* l) 
	: Fl_Gl_Window(x,y,w,h,l)
//========================================================================
{
	mode( FL_RGB|FL_ALPHA|FL_DOUBLE | FL_STENCIL );

	resetArcball();
}

//************************************************************************
//
// * Reset the camera to look at the world
//========================================================================
void TrainView::
resetArcball()
//========================================================================
{
	// Set up the camera to look at the world
	// these parameters might seem magical, and they kindof are
	// a little trial and error goes a long way
	arcball.setup(this, 40, 250, .2f, .4f, 0);
}

//************************************************************************
//
// * FlTk Event handler for the window
//########################################################################
// TODO: 
//       if you want to make the train respond to other events 
//       (like key presses), you might want to hack this.
//########################################################################
//========================================================================
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, 
	// then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->worldCam->value())
		if (arcball.handle(event)) 
			return 1;

	// remember what button was used
	static int last_push;

	switch(event) {
		// Mouse button being pushed event
		case FL_PUSH:
			last_push = Fl::event_button();
			// if the left button be pushed is left mouse button
			if (last_push == FL_LEFT_MOUSE  ) {
				doPick();
				damage(1);
				return 1;
			};
			break;

	   // Mouse button release event
		case FL_RELEASE: // button release
			damage(1);
			last_push = 0;
			return 1;

		// Mouse button drag event
		case FL_DRAG:

			// Compute the new control point position
			if ((last_push == FL_LEFT_MOUSE) && (selectedCube >= 0)) {
				ControlPoint* cp = &m_pTrack->points[selectedCube];

				double r1x, r1y, r1z, r2x, r2y, r2z;
				getMouseLine(r1x, r1y, r1z, r2x, r2y, r2z);

				double rx, ry, rz;
				mousePoleGo(r1x, r1y, r1z, r2x, r2y, r2z, 
								static_cast<double>(cp->pos.x), 
								static_cast<double>(cp->pos.y),
								static_cast<double>(cp->pos.z),
								rx, ry, rz,
								(Fl::event_state() & FL_CTRL) != 0);

				cp->pos.x = (float) rx;
				cp->pos.y = (float) ry;
				cp->pos.z = (float) rz;
				damage(1);
			}
			break;

		// in order to get keyboard events, we need to accept focus
		case FL_FOCUS:
			return 1;

		// every time the mouse enters this window, aggressively take focus
		case FL_ENTER:	
			focus(this);
			break;

		case FL_KEYBOARD:
		 		int k = Fl::event_key();
				int ks = Fl::event_state();
				if (k == 'p') {
					// Print out the selected control point information
					if (selectedCube >= 0) 
						printf("Selected(%d) (%g %g %g) (%g %g %g)\n",
								 selectedCube,
								 m_pTrack->points[selectedCube].pos.x,
								 m_pTrack->points[selectedCube].pos.y,
								 m_pTrack->points[selectedCube].pos.z,
								 m_pTrack->points[selectedCube].orient.x,
								 m_pTrack->points[selectedCube].orient.y,
								 m_pTrack->points[selectedCube].orient.z);
					else
						printf("Nothing Selected\n");

					return 1;
				};
				break;
	}

	return Fl_Gl_Window::handle(event);
}

//************************************************************************
//
// * this is the code that actually draws the window
//   it puts a lot of the work into other routines to simplify things
//========================================================================
void TrainView::draw()
{

	//*********************************************************************
	//
	// * Set up basic opengl informaiton
	//
	//**********************************************************************
	//initialized glad
	if (gladLoadGL())
	{
		//initiailize VAO, VBO, Shader...
		if (!this->cup_shader)
		{
			string path = "Models/blue_cup.obj";
			this->blue_cup = new Model(path);
			path = "Models/red_cup.obj";
			this->red_cup = new Model(path);
			path = "Models/green_cup.obj";
			this->green_cup = new Model(path);
			path = "Models/yellow_cup.obj";
			this->yellow_cup = new Model(path);
			this->cup_shader = new Shader( "src/shaders/cup.vert",
				nullptr, nullptr, nullptr,
				 "src/shaders/cup.frag");
		}
		if (!this->cup_base_shader)
		{
			string path = "Models/base.obj";
			this->cup_base = new Model(path);
			this->cup_base_shader = new Shader( "src/shaders/cup_base.vert",
				nullptr, nullptr, nullptr,
				 "src/shaders/cup_base.frag");
		}
		if (!this->teapot_shader)
		{
			string path = "Models/teapot.obj";
			this->teapot = new Model(path);
			this->teapot_shader = new Shader( "src/shaders/teapot.vert",
				nullptr, nullptr, nullptr,
				 "src/shaders/teapot.frag");
		}
		if (!this->ferris_wheel_shader)
		{
			string path = "Models/ferris_wheel_main.obj";
			this->ferris_wheel_main = new Model(path);
			path = "Models/wheel.obj";
			this->wheel = new Model(path);
			path = "Models/car.obj";
			this->car = new Model(path);
			this->car_red= new Texture2D( "Models/car_red.jpg");
			this->car_orange = new Texture2D( "Models/car_orange.jpg");
			this->car_yellow = new Texture2D( "Models/car_yellow.jpg");
			this->car_green = new Texture2D( "Models/car_green.jpg");
			this->car_blue = new Texture2D( "Models/car_blue.jpg");
			this->car_blue2 = new Texture2D( "Models/car_blue2.jpg");
			this->car_purple = new Texture2D( "Models/car_purple.jpg");
			this->car_pink = new Texture2D( "Models/car_pink.jpg");
			this->ferris_wheel_shader = new Shader( "src/shaders/ferris_wheel.vert",
				nullptr, nullptr, nullptr,
				 "src/shaders/ferris_wheel.frag");
		}
		if (!this->water_slide)
		{
			string path = "Models/water_slide.obj";
			this->water_slide = new Model(path);
			this->water_slide_shader = new Shader( "src/shaders/water_slide.vert",
				nullptr, nullptr, nullptr,
				 "src/shaders/water_slide.frag");
		}
		if (!this->water_shader)
		{
			string path = "Models/water.obj";
			this->water = new Model(path);
			this->water_shader = new Shader( "src/shaders/water.vert",
				nullptr, nullptr, nullptr,
				 "src/shaders/water.frag");
			for (int i = 0; i < 200; i++)
			{
				std::string str = "Images/waves/";
				if (i < 10)
				{
					str += ("00" + std::to_string(i) + ".png");
				}
				else if (i < 100)
				{
					str += ("0" + std::to_string(i) + ".png");
				}
				else
				{
					str += (std::to_string(i) + ".png");
				}
				this->height_map[i] = new Texture2D(str.c_str());
			}
		}
		if (!this->skybox_shader) {
			//r l t b back front
			vector<string> paths(6);
			paths[0] = "Images/skybox/white_sky/Right_Tex.png";
			paths[1] = "Images/skybox/white_sky/Left_Tex.png";
			paths[2] = "Images/skybox/white_sky/Up_Tex.png";
			paths[3] = "Images/skybox/white_sky/Down_Tex.png";
			paths[4] = "Images/skybox/white_sky/Back_Tex.png";
			paths[5] = "Images/skybox/white_sky/Front_Tex.png";

			this->loadSkyBox(this->skybox_whitesky, paths);
			paths[0] = "Images/skybox/stars/Right_Tex.png";
			paths[1] = "Images/skybox/stars/Left_Tex.png";
			paths[2] = "Images/skybox/stars/Up_Tex.png";
			paths[3] = "Images/skybox/stars/Down_Tex.png";
			paths[4] = "Images/skybox/stars/Back_Tex.png";
			paths[5] = "Images/skybox/stars/Front_Tex.png";
			this->loadSkyBox(this->skybox_stars_sky, paths);
			this->skybox_shader = new Shader("src/shaders/cubemap.vert", nullptr, nullptr, nullptr
				, "src/shaders/cubemap.frag");
		}
		if (!this->test_ani) {
			this->test_ani = new AniModel("Models/dae_files/model.dae", "Models/dae_files/diffuse.png");
			this->test_shader_ani = new Shader("src/shaders/just_ani.vert", nullptr, nullptr, nullptr, "src/shaders/just_ani.frag");
		}
		if (!this->cowboy_sit) {
			this->cowboy_sit = new Model("Models/spinning_cup/boyfriendNeedsHelp_sit.dae");
			this->cowboy_sit_shader = new Shader("src/shaders/just_load.vert", nullptr, nullptr, nullptr,
				"src/shaders/just_load.frag");
			if (!this->cowboy_sit_handsUp) {
				this->cowboy_sit_handsUp = new Model("Models/spinning_cup/boyfriendNeedsHelp_sit_handsDown.dae");
				this->cowboy_sit_shader_handsUp = new Shader("src/shaders/just_load.vert", nullptr, nullptr, nullptr,
					"src/shaders/just_load.frag");
			}
			Texture cowboy;
			cv::Mat img;

			//cv::imread(path, cv::IMREAD_COLOR).convertTo(img, CV_32FC3, 1 / 255.0f);	//unsigned char to float
			img = cv::imread("Models/spinning_cup/diffuse.png", cv::IMREAD_COLOR);
			//cv::cvtColor(img, img, CV_BGR2RGB);
			if (img.data) {
				glGenTextures(1, &cowboy.id);

				glBindTexture(GL_TEXTURE_2D, cowboy.id);
				glGenerateMipmap(GL_TEXTURE_2D);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

				if (img.type() == CV_8UC3)
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, img.cols, img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, img.data);
				else if (img.type() == CV_8UC4)
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.cols, img.rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, img.data);
				glBindTexture(GL_TEXTURE_2D, 0);

				img.release();
				cowboy.type = "texture_diffuse";
				this->cowboy_sit->meshes[0].textures.push_back(cowboy);
				this->cowboy_sit_handsUp->meshes[0].textures.push_back(cowboy);
			}
			else {
				cout << "Error!!!!! img loading bad" << endl;
			}
		}
		if (!this->psystem) {
			this->psystem = new ParticleSystem();
			if (!this->particle_shader) {
				this->particle_shader = new Shader("src/shaders/particle.vert", nullptr, nullptr, nullptr,
					"src/shaders/particle.frag");
			}
		}
		if (!this->floor) {
			this->floor = new Model("Models/floors/floor.obj");
			cout << "water loading success!" << endl;
		}
		if (!this->floor_shader) {
			this->floor_shader = new
				Shader(
					 "src/shaders/floor.vert",
					nullptr, nullptr, nullptr,
					 "src/shaders/floor.frag");
		}
		if (!this->drop_tower_shader)
		{
			string path = "Models/drop_tower.obj";
			this->drop_tower = new Model(path);
			path= "Models/drop_tower_seat.obj";
			this->drop_tower_seat = new Model(path);
			this->drop_tower_shader = new Shader("src/shaders/drop_tower.vert",
				nullptr, nullptr, nullptr,
				 "src/shaders/drop_tower.frag");
		}
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");

	// Set up the view port
	glViewport(0,0,w(),h());

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0,0,.3f,0);		// background should be blue

	// we need to clear out the stencil buffer since we'll use
	// it for shadows
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH);

	// Blayne prefers GL_DIFFUSE
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// prepare for projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	setProjection();		// put the code to set up matrices here

	//######################################################################
	// TODO: 
	// you might want to set the lighting up differently. if you do, 
	// we need to set up the lights AFTER setting up the projection
	//######################################################################
	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// top view only needs one light

		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);


	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	glDisable(GL_LIGHTING);
	//drawFloor(400,10);


	//*********************************************************************
	// now draw the object and we need to do it twice
	// once for real, and then once for shadows
	//*********************************************************************
	//glEnable(GL_LIGHTING);
	setupObjects();

	drawStuff();


	this->drawTrack(this);
	this->drawTiles();
	if (tw->cameraBrowser->value()!=2)
	{
		this->drawTrain(this);
	}
	this->drawCup(this->blue_cup);
	this->drawCup(this->red_cup);
	this->drawCup(this->green_cup);
	this->drawCup(this->yellow_cup);
	this->drawCupBase();
	this->drawTeapot();
	this->drawFerrisWheelMain();
	this->drawWheel();
	this->drawCar(RED);
	this->drawCar(ORANGE);
	this->drawCar(YELLOW);
	this->drawCar(GREEN);
	this->drawCar(BLUE);
	this->drawCar(BLUE2);
	this->drawCar(PURPLE);
	this->drawCar(PINK);
	this->drawSkybox();
	this->drawDropTower();
	this->drawDropTowerSeat();
	
	glEnable(GL_BLEND);
	this->drawWaterSlide();
	glDepthMask(GL_FALSE);
	this->drawWater();
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);


	if (1) {
		if (tw->runButton->value() == 0) {
			this->cowboy_sit_shader_handsUp->Use();
			float angle =270;
			glm::vec3 position = glm::vec3(100, 2.5, -15);
			
			glm::vec3 a = position - glm::vec3(100, 2.5, 0);
			glm::mat4 mat = glm::rotate(glm::radians(this->time), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::vec3 b = mat * glm::vec4(a, 1.0);

			glm::vec3 position2;
			position2 = glm::vec3(100, 2.5, -13);
			glm::vec3 a2 = position2 - position;
			glm::mat4 mat2 = glm::rotate(glm::radians(this->time * -6), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::vec3 b2 = mat2 * glm::vec4(a2, 1.0);


			glm::mat4 model_matrix;
			//model_matrix = glm::scale(model_matrix, glm::vec3(1, 1, 1));
			
			model_matrix = glm::translate(model_matrix, glm::vec3(100, 2.5, 0));
			model_matrix = glm::translate(model_matrix, b);
			model_matrix = glm::translate(model_matrix, b2);
			model_matrix = glm::rotate(model_matrix, glm::radians(this->time*-6+180), glm::vec3(0, 1, 0));

			model_matrix = glm::rotate(model_matrix, glm::radians(angle), glm::vec3(1, 0, 0));



			glm::mat4 view_matrix, project_matrix;
			glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
			glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);

			glUniformMatrix4fv(
				glGetUniformLocation(this->cowboy_sit_shader_handsUp->Program, "model"), 1, GL_FALSE, &model_matrix[0][0]);
			glUniformMatrix4fv(
				glGetUniformLocation(this->cowboy_sit_shader_handsUp->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
			glUniformMatrix4fv(
				glGetUniformLocation(this->cowboy_sit_shader_handsUp->Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);

			this->cowboy_sit_handsUp->Draw(this->cowboy_sit_shader_handsUp);
			//unbind VAO
			glBindVertexArray(0);

			//unbind shader(switch to fixed pipeline)
			glUseProgram(0);
		}
		else {
			this->cowboy_sit_shader->Use();
			float angle = 270;
			glm::vec3 position = glm::vec3(100, 2.5, -15);

			glm::vec3 a = position - glm::vec3(100, 2.5, 0);
			glm::mat4 mat = glm::rotate(glm::radians(this->time), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::vec3 b = mat * glm::vec4(a, 1.0);

			glm::vec3 position2;
			position2 = glm::vec3(100, 2.5, -13);
			glm::vec3 a2 = position2 - position;
			glm::mat4 mat2 = glm::rotate(glm::radians(this->time * -6+180), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::vec3 b2 = mat2 * glm::vec4(a2, 1.0);


			glm::mat4 model_matrix;

			model_matrix = glm::translate(model_matrix, glm::vec3(100, 2.5, 0));
			model_matrix = glm::translate(model_matrix, b);
			model_matrix = glm::translate(model_matrix, b2);
			model_matrix = glm::rotate(model_matrix, glm::radians(this->time * -6), glm::vec3(0, 1, 0));
			model_matrix = glm::rotate(model_matrix, glm::radians(angle), glm::vec3(1, 0, 0));

			glm::mat4 view_matrix, project_matrix;
			glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
			glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);

			glUniformMatrix4fv(
				glGetUniformLocation(this->cowboy_sit_shader->Program, "model"), 1, GL_FALSE, &model_matrix[0][0]);
			glUniformMatrix4fv(
				glGetUniformLocation(this->cowboy_sit_shader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
			glUniformMatrix4fv(
				glGetUniformLocation(this->cowboy_sit_shader->Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);

			this->cowboy_sit->Draw(this->cowboy_sit_shader);
			//unbind VAO
			glBindVertexArray(0);

			//unbind shader(switch to fixed pipeline)
			glUseProgram(0);
		}
	}
	if (1)  //test_ani
	{
		this->test_shader_ani->Use();
		glm::vec3 position = glm::vec3(70, 0,0);

		glm::vec3 a = position - glm::vec3(100, 0, 0);
		glm::mat4 mat = glm::rotate(glm::radians(this->time), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec3 b = mat * glm::vec4(a, 1.0);

		glm::mat4 model_matrix = glm::mat4();
		float angle = 135;
		
		model_matrix = glm::rotate(glm::radians(this->time), glm::vec3(0.0f, 1.0f, 0.0f));
		model_matrix = glm::translate(model_matrix, glm::vec3(100, 0, 0));
		model_matrix = glm::translate(model_matrix, b);
	
		model_matrix = glm::rotate(model_matrix, angle, glm::vec3(0, 0, 1));

		
	

		glm::mat4 view_matrix, project_matrix;
		glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
		glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->test_shader_ani->Program, "view_projection_matrix"), 1, GL_FALSE, &(project_matrix * view_matrix)[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->test_shader_ani->Program, "model_matrix"), 1, GL_FALSE, &(model_matrix[0][0]));
		glm::mat4 view;
		glGetFloatv(GL_MODELVIEW_MATRIX, &view[0][0]);
		glm::mat4 inversion = glm::inverse(view);
		glm::vec3 viewerPos(inversion[3][0], inversion[3][1], inversion[3][2]);
		glUniform3f(glGetUniformLocation(this->test_shader_ani->Program, "dirLight.direction"), 0, -1.0f, 0);
		glUniform3f(glGetUniformLocation(this->test_shader_ani->Program, "dirLight.ambient"), 0.1, 0.1, 0.1);
		glUniform3f(glGetUniformLocation(this->test_shader_ani->Program, "dirLight.diffuse"), 0.5, 0.5, 0.5);
		glUniform3f(glGetUniformLocation(this->test_shader_ani->Program, "dirLight.specular"), 0.5, 0.5, 0.5);

		glUniform3f(glGetUniformLocation(this->test_shader_ani->Program, "viewPos"), viewerPos.x, viewerPos.y, viewerPos.z);
		//float elapsedTime = 0.2;

		this->test_ani->Draw(*test_shader_ani, 0.015*time);

		//unbind VAO
		glBindVertexArray(0);

		//unbind shader(switch to fixed pipeline)
		glUseProgram(0);
	}
	//particle 
	if (tw->particleType->value()>=1) {
		if (this->psystem->particles.size() == 0) {
			for (int i = 0; i < 1; ++i) {
				this->psystem->particles.push_back(Particle(glm::vec3(0, 5, 0), glm::vec3(0, 10, 0), 0, 5, 0, 5));
				this->psystem->particles[i].setType(tw->particleType->value());
				this->psystem->particles[i].setColor(glm::vec3(1, 1, 1));
			}
		}
		this->psystem->update();
		this->psystem->renderParticles(*this->particle_shader);
	}

	//rendering floor
	if (1)
	{
		//bind shader
		this->floor_shader->Use();

		glm::mat4 model_matrix = glm::mat4();
		//model_matrix = glm::translate(model_matrix, glm::vec3(0, 0, 0));
		model_matrix = glm::scale(model_matrix, glm::vec3(150, 150, 150));

		glm::mat4 view_matrix, project_matrix;
		glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
		glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);

		glUniformMatrix4fv(
			glGetUniformLocation(this->floor_shader->Program, "model"), 1, GL_FALSE, &model_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->floor_shader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->floor_shader->Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);
		this->floor->Draw(floor_shader);

		//unbind VAO
		glBindVertexArray(0);

		//unbind shader(switch to fixed pipeline)
		glUseProgram(0);
	}
	

}

//************************************************************************
//
// * This sets up both the Projection and the ModelView matrices
//   HOWEVER: it doesn't clear the projection first (the caller handles
//   that) - its important for picking
//========================================================================
void TrainView::
setProjection()
//========================================================================
{
	// Compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());

	// Check whether we use the world camp
	if (tw->cameraBrowser->value()==1 || tw->cameraBrowser->value() == 0)
		arcball.setProjection(false);
	// Or we use the top cam
	else if (tw->cameraBrowser->value()==3) {
		float wi, he;
		if (aspect >= 1) {
			wi = 110;
			he = wi / aspect;
		}
		else {
			he = 110;
			wi = he * aspect;
		}

		// Set up the top camera drop mode to be orthogonal and set
		// up proper projection matrix
		glMatrixMode(GL_PROJECTION);
		glOrtho(-wi, wi, -he, he, 200, -200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90, 1, 0, 0);
	}
	else if (tw->cameraBrowser->value()==2)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(30, aspect, 0.001, 500);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		int i = 0;
		float t = 0;
		if (this->trainU > this->totalArc)
		{
			this->trainU -= this->totalArc;
		}
		double tmp = 0;
		int count = 0;
		double position = this->trainU;
		if (position < 0)
		{
			position += this->totalArc;
		}
		while (position > tmp)
		{
			tmp += this->arc_length[count];
			count++;
		}
		if (tmp == position)
		{
			i = (count - 1) / 20;
			t = ((count - 1) % 20) * 0.05;
		}
		else
		{
			float x = (position - (tmp - this->arc_length[count - 1])) / ((tmp - position) + (position - (tmp - this->arc_length[count - 1])));
			i = (count - 1) / 20;
			t = ((count - 1) % 20 + x) * 0.05;
		}
		ControlPoint p0 = m_pTrack->points[i % m_pTrack->points.size()];
		ControlPoint p1 = m_pTrack->points[(i + 1) % m_pTrack->points.size()];
		ControlPoint p2 = m_pTrack->points[(i + 2) % m_pTrack->points.size()];
		ControlPoint p3 = m_pTrack->points[(i + 3) % m_pTrack->points.size()];
		Pnt3f qt = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), t);
		Pnt3f nextQt = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), t + 1.0 / DIVIDE_LINE);
		Pnt3f ori = GMT(p0.orient, p1.orient, p2.orient, p3.orient, tw->splineBrowser->value(), t);
		Pnt3f forward = nextQt - qt;
		forward.normalize();
		Pnt3f cross_t = forward * ori;
		cross_t.normalize();
		Pnt3f up = cross_t * forward;
		up.normalize();
		this->trainAcc = forward.y * tw->speed->value() * -1;
		Pnt3f pos = qt + up * 5.0f;
		Pnt3f nextPos = nextQt + up * 5.0f;

		gluLookAt(pos.x, pos.y, pos.z, nextPos.x, nextPos.y, nextPos.z, ori.x, ori.y, ori.z);
	}
	else if (tw->cameraBrowser->value()==6)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(60, aspect, 0.01, 500);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glm::vec3 position;
		position = glm::vec3(85, 7, 0);
		glm::vec3 a = position - glm::vec3(100, 7, 0);
		glm::mat4 mat = glm::rotate(glm::radians(this->time), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec3 b = mat * glm::vec4(a, 1.0);

		glm::vec3 position2;
		position2 = glm::vec3(84, 7, 0);
		glm::vec3 a2 = position2 - position;
		glm::mat4 mat2 = glm::rotate(glm::radians(this->time*-4), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec3 b2 = mat2 * glm::vec4(a2, 1.0);

		Pnt3f pos((glm::vec3(100, 7, 0) + b+b2).x, (glm::vec3(100, 7, 0) + b+b2).y, (glm::vec3(100, 7, 0) + b+b2).z);
		//Pnt3f vec((glm::vec3(, 6, 0) + b).x, (glm::vec3(100, 6, 0) + b).y, (glm::vec3(100, 6, 0) + b).z);
		gluLookAt(pos.x, pos.y, pos.z, (glm::vec3(100, 7, 0) + b).x, (glm::vec3(100, 7, 0) + b).y, (glm::vec3(100, 7, 0) + b).z, 0, 1, 0);

	}
	else if (tw->cameraBrowser->value()==4)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45, aspect, 0.01, 500);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glm::vec3 position;
		position = glm::vec3(-35.36, 29.63, -30);
		glm::vec3 a = position - glm::vec3(0, 65, -30);
		glm::mat4 mat = glm::rotate(glm::radians(-1 * this->time), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::vec3 b = mat * glm::vec4(a, 1.0);
		Pnt3f pos((glm::vec3(0, 65, -30) + b).x, (glm::vec3(0, 65, -30) + b).y, (glm::vec3(0, 65, -30) + b).z);
		gluLookAt(pos.x - 4, pos.y - 5, pos.z + 0.5, pos.x + 100, pos.y - 6, pos.z + 0.5, 0, 1, 0);
	}
	else if (tw->cameraBrowser->value() == 5)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45, aspect, 0.01, 1000);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glm::vec3 position;
		position = glm::vec3(-35.36, 29.63, -30);
		glm::vec3 a = position - glm::vec3(0, 65, -30);
		glm::mat4 mat = glm::rotate(glm::radians(-1 * this->time), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::vec3 b = mat * glm::vec4(a, 1.0);
		Pnt3f pos((glm::vec3(0, 65, -30) + b).x, (glm::vec3(0, 65, -30) + b).y, (glm::vec3(0, 65, -30) + b).z);
		gluLookAt(pos.x - 4, pos.y - 5, pos.z + 0.5, pos.x, pos.y - 6, pos.z + 0.5 + 100, 0, 1, 0);
	}

	// Or do the train view or other view here
	//####################################################################
	// TODO: 
	// put code for train view projection here!	
	//####################################################################
	else {
#ifdef EXAMPLE_SOLUTION
		trainCamView(this, aspect);
#endif
	}
}

//************************************************************************
//
// * this draws all of the stuff in the world
//
//	NOTE: if you're drawing shadows, DO NOT set colors (otherwise, you get 
//       colored shadows). this gets called twice per draw 
//       -- once for the objects, once for the shadows
//########################################################################
// TODO: 
// if you have other objects in the world, make sure to draw them
//########################################################################
//========================================================================
void TrainView::drawStuff(bool doingShadows)
{
	// Draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (0) {
		for(size_t i=0; i<m_pTrack->points.size(); ++i) {
			if (!doingShadows) {
				if ( ((int) i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			m_pTrack->points[i].draw();
		}
	}
	// draw the track
	//####################################################################
	// TODO: 
	// call your own track drawing code
	//####################################################################

#ifdef EXAMPLE_SOLUTION
	drawTrack(this, doingShadows);
#endif

	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################
#ifdef EXAMPLE_SOLUTION
	// don't draw the train if you're looking out the front window
	if (!tw->trainCam->value())
		drawTrain(this, doingShadows);
#endif
}

// 
//************************************************************************
//
// * this tries to see which control point is under the mouse
//	  (for when the mouse is clicked)
//		it uses OpenGL picking - which is always a trick
//########################################################################
// TODO: 
//		if you want to pick things other than control points, or you
//		changed how control points are drawn, you might need to change this
//########################################################################
//========================================================================
void TrainView::
doPick()
//========================================================================
{
	// since we'll need to do some GL stuff so we make this window as 
	// active window
	make_current();		

	// where is the mouse?
	int mx = Fl::event_x(); 
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	gluPickMatrix((double)mx, (double)(viewport[3]-my), 
						5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100,buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for(size_t i=0; i<m_pTrack->points.size(); ++i) {
		glLoadName((GLuint) (i+1));
		m_pTrack->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3]-1;
	} else // nothing hit, nothing selected
		selectedCube = -1;

	printf("Selected Cube %d\n",selectedCube);
}

void TrainView::setUBO()
{
	float wdt = this->pixel_w();
	float hgt = this->pixel_h();

	glm::mat4 view_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	//HMatrix view_matrix; 
	//this->arcball.getMatrix(view_matrix);

	glm::mat4 projection_matrix;
	glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);
	//projection_matrix = glm::perspective(glm::radians(this->arcball.getFoV()), (GLfloat)wdt / (GLfloat)hgt, 0.01f, 1000.0f);

	glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &projection_matrix[0][0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &view_matrix[0][0]);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void TrainView::drawCup(Model* cup)
{
	glm::vec3 position;
	float speed=1;
	if (cup == this->blue_cup)
	{
		position=glm::vec3(85, 2.5, 0);
		speed = -4;
	}
	else if (cup == this->red_cup)
	{
		position = glm::vec3(100, 2.5, -15);
		speed = -6;
	}
	else if (cup == this->green_cup)
	{
		position = glm::vec3(115, 2.5, 0);
		speed = 3;
	}
	else if (cup == this->yellow_cup)
	{
		position = glm::vec3(100, 2.5, 15);
		speed = 7;
	}
	glm::vec3 a = position - glm::vec3(100, 2.5, 0);
	glm::mat4 mat = glm::rotate(glm::radians(this->time), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 b = mat * glm::vec4(a, 1.0);
	glm::mat4 model_matrix = glm::mat4();

	
	model_matrix = glm::translate(model_matrix, glm::vec3(100, 2.5, 0));
	model_matrix = glm::translate(model_matrix, b);
	model_matrix = glm::scale(model_matrix, glm::vec3(70, 70, 70));
	model_matrix = glm::rotate(model_matrix, glm::radians(speed * this->time), glm::vec3(0, 1, 0));

	glm::mat4 view_matrix, project_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);

	glm::mat4 view;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view[0][0]);
	glm::mat4 inversion = glm::inverse(view);
	glm::vec3 viewerPos(inversion[3][0], inversion[3][1], inversion[3][2]);
	this->cup_shader->Use();
	for (int j = 0; j <cup->meshes.size(); j++)
	{
		for (unsigned int i = 0; i < cup->meshes[j].textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
			glUniform1i(glGetUniformLocation(this->cup_shader->Program,"u_texture"), i);
			//shader.setFloat(("material." + name + number).c_str(), i);
			glBindTexture(GL_TEXTURE_2D, cup->meshes[j].textures[i].id);
		}
		glActiveTexture(GL_TEXTURE0);
		glUniformMatrix4fv(
			glGetUniformLocation(this->cup_shader->Program, "model"), 1, GL_FALSE, &model_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->cup_shader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->cup_shader->Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);

		glUniform3f(glGetUniformLocation(this->cup_shader->Program, "dirLight.direction"), 0, -1.0f, 0);
		glUniform3f(glGetUniformLocation(this->cup_shader->Program, "dirLight.ambient"), 0.1, 0.1, 0.1);
		glUniform3f(glGetUniformLocation(this->cup_shader->Program, "dirLight.diffuse"), 0.5, 0.5, 0.5);
		glUniform3f(glGetUniformLocation(this->cup_shader->Program, "dirLight.specular"), 1.0, 1.0, 1.0);

		glUniform3f(glGetUniformLocation(this->cup_shader->Program, "viewPos"), viewerPos.x, viewerPos.y, viewerPos.z);

		// draw mesh
		glBindVertexArray(cup->meshes[j].VAO);
		glDrawElements(GL_TRIANGLES, cup->meshes[j].indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}
}

void TrainView::drawCupBase()
{
	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, glm::vec3(100, 0, 0));
	model_matrix = glm::rotate(model_matrix, glm::radians(this->time), glm::vec3(0, 1, 0));
	model_matrix = glm::scale(model_matrix, glm::vec3(75, 60, 75));

	glm::mat4 view_matrix, project_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);

	glm::mat4 view;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view[0][0]);
	glm::mat4 inversion = glm::inverse(view);
	glm::vec3 viewerPos(inversion[3][0], inversion[3][1], inversion[3][2]);

	this->cup_base_shader->Use();
	for (int j = 0; j < this->cup_base->meshes.size(); j++)
	{
		for (unsigned int i = 0; i < this->cup_base->meshes[j].textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
			glUniform1i(glGetUniformLocation(this->cup_base_shader->Program, "u_texture"), i);
			//shader.setFloat(("material." + name + number).c_str(), i);
			glBindTexture(GL_TEXTURE_2D, this->cup_base->meshes[j].textures[i].id);
		}
		glUniformMatrix4fv(
			glGetUniformLocation(this->cup_base_shader->Program, "model"), 1, GL_FALSE, &model_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->cup_base_shader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->cup_base_shader->Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);

		glUniform3f(glGetUniformLocation(this->cup_base_shader->Program, "dirLight.direction"), 0, -1.0f, 0);
		glUniform3f(glGetUniformLocation(this->cup_base_shader->Program, "dirLight.ambient"), 0.1, 0.1, 0.1);
		glUniform3f(glGetUniformLocation(this->cup_base_shader->Program, "dirLight.diffuse"), 0.5, 0.5, 0.5);
		glUniform3f(glGetUniformLocation(this->cup_base_shader->Program, "dirLight.specular"), 0.1, 0.1, 0.1);

		glUniform3f(glGetUniformLocation(this->cup_base_shader->Program, "viewPos"), viewerPos.x, viewerPos.y, viewerPos.z);
		glActiveTexture(GL_TEXTURE0);

		// draw mesh
		glBindVertexArray(this->cup_base->meshes[j].VAO);
		glDrawElements(GL_TRIANGLES, this->cup_base->meshes[j].indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}
}

void TrainView::drawTeapot()
{
	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, glm::vec3(100, 15, 0));
	model_matrix = glm::rotate(model_matrix, glm::radians(this->time), glm::vec3(0, 1, 0));
	model_matrix = glm::scale(model_matrix, glm::vec3(30,30,30));

	glm::mat4 view_matrix, project_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);

	glm::mat4 view;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view[0][0]);
	glm::mat4 inversion = glm::inverse(view);
	glm::vec3 viewerPos(inversion[3][0], inversion[3][1], inversion[3][2]);

	this->teapot_shader->Use();
	for (int j = 0; j < this->teapot->meshes.size(); j++)
	{
		for (unsigned int i = 0; i < this->teapot->meshes[j].textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
			glUniform1i(glGetUniformLocation(this->teapot_shader->Program, "u_texture"), i);
			//shader.setFloat(("material." + name + number).c_str(), i);
			glBindTexture(GL_TEXTURE_2D, this->teapot->meshes[j].textures[i].id);
		}
		glUniformMatrix4fv(
			glGetUniformLocation(this->teapot_shader->Program, "model"), 1, GL_FALSE, &model_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->teapot_shader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->teapot_shader->Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);

		glUniform3f(glGetUniformLocation(this->teapot_shader->Program, "dirLight.direction"), 0, -1.0f, 0);
		glUniform3f(glGetUniformLocation(this->teapot_shader->Program, "dirLight.ambient"), 0.1, 0.1, 0.1);
		glUniform3f(glGetUniformLocation(this->teapot_shader->Program, "dirLight.diffuse"), 0.5, 0.5, 0.5);
		glUniform3f(glGetUniformLocation(this->teapot_shader->Program, "dirLight.specular"), 0.1, 0.1, 0.1);

		glUniform3f(glGetUniformLocation(this->teapot_shader->Program, "viewPos"), viewerPos.x, viewerPos.y, viewerPos.z);
		glActiveTexture(GL_TEXTURE0);

		// draw mesh
		glBindVertexArray(this->teapot->meshes[j].VAO);
		glDrawElements(GL_TRIANGLES, this->teapot->meshes[j].indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}
}

void TrainView::drawFerrisWheelMain()
{
	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, glm::vec3(0, 15, -30));
	model_matrix = glm::scale(model_matrix, glm::vec3(5, 5, 5.5));

	glm::mat4 view_matrix, project_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);

	glm::mat4 view;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view[0][0]);
	glm::mat4 inversion = glm::inverse(view);
	glm::vec3 viewerPos(inversion[3][0], inversion[3][1], inversion[3][2]);

	this->ferris_wheel_shader->Use();
	for (int j = 0; j < this->ferris_wheel_main->meshes.size(); j++)
	{
		for (unsigned int i = 0; i < this->ferris_wheel_main->meshes[j].textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
			glUniform1i(glGetUniformLocation(this->ferris_wheel_shader->Program, "u_texture"), i);
			glBindTexture(GL_TEXTURE_2D, this->ferris_wheel_main->meshes[j].textures[i].id);
		}
		glUniformMatrix4fv(
			glGetUniformLocation(this->ferris_wheel_shader->Program, "model"), 1, GL_FALSE, &model_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->ferris_wheel_shader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->ferris_wheel_shader->Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);

		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "dirLight.direction"), 0, -1.0f, 0);
		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "dirLight.ambient"), 0.1, 0.1, 0.1);
		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "dirLight.diffuse"), 0.5, 0.5, 0.5);
		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "dirLight.specular"), 0.3, 0.3, 0.3);

		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "viewPos"), viewerPos.x, viewerPos.y, viewerPos.z);
		glActiveTexture(GL_TEXTURE0);

		// draw mesh
		glBindVertexArray(this->ferris_wheel_main->meshes[j].VAO);
		glDrawElements(GL_TRIANGLES, this->ferris_wheel_main->meshes[j].indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}
}

void TrainView::drawWheel()
{
	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, glm::vec3(0, 65, -30));
	model_matrix = glm::rotate(model_matrix, glm::radians(-1 * this->time), glm::vec3(0, 0, 1));
	model_matrix = glm::scale(model_matrix, glm::vec3(5, 5, 6));

	glm::mat4 view_matrix, project_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);

	glm::mat4 view;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view[0][0]);
	glm::mat4 inversion = glm::inverse(view);
	glm::vec3 viewerPos(inversion[3][0], inversion[3][1], inversion[3][2]);

	this->ferris_wheel_shader->Use();
	for (int j = 0; j < this->wheel->meshes.size(); j++)
	{
		for (unsigned int i = 0; i < this->wheel->meshes[j].textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
			glUniform1i(glGetUniformLocation(this->ferris_wheel_shader->Program, "u_texture"), i);
			//shader.setFloat(("material." + name + number).c_str(), i);
			glBindTexture(GL_TEXTURE_2D, this->wheel->meshes[j].textures[i].id);
		}
		glUniformMatrix4fv(
			glGetUniformLocation(this->ferris_wheel_shader->Program, "model"), 1, GL_FALSE, &model_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->ferris_wheel_shader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->ferris_wheel_shader->Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);

		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "dirLight.direction"), 0, -1.0f, 0);
		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "dirLight.ambient"), 0.1, 0.1, 0.1);
		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "dirLight.diffuse"), 0.5, 0.5, 0.5);
		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "dirLight.specular"), 1.0, 1.0, 1.0);

		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "viewPos"), viewerPos.x, viewerPos.y, viewerPos.z);
		glActiveTexture(GL_TEXTURE0);

		// draw mesh
		glBindVertexArray(this->wheel->meshes[j].VAO);
		glDrawElements(GL_TRIANGLES, this->wheel->meshes[j].indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}
}

void TrainView::drawCar(int color)
{
	glm::vec3 position;
	switch (color) {
	case RED:
		position = glm::vec3(0, 15, -30);
		break;
	case ORANGE:
		position = glm::vec3(-35.36, 29.63, -30);
		break;
	case YELLOW:
		position = glm::vec3(-50, 65, -30);
		break;
	case GREEN:
		position = glm::vec3(-35.36, 100.36, -30);
		break;
	case BLUE:
		position = glm::vec3(0, 115, -30);
		break;
	case BLUE2:
		position = glm::vec3(35.36, 100.36, -30);
		break;
	case PURPLE:
		position = glm::vec3(50, 65, -30);
		break;
	case PINK:
		position = glm::vec3(35.36, 29.63, -30);
		break;
	}
	glm::vec3 a = position - glm::vec3(0, 65, -30);
	glm::mat4 mat = glm::rotate(glm::radians(-1 * this->time), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::vec3 b = mat * glm::vec4(a, 1.0);
	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, glm::vec3(0, 65, -30));
	model_matrix = glm::translate(model_matrix, b);
	model_matrix = glm::scale(model_matrix, glm::vec3(5, 5, 5));

	glm::mat4 view_matrix, project_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);

	glm::mat4 view;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view[0][0]);
	glm::mat4 inversion = glm::inverse(view);
	glm::vec3 viewerPos(inversion[3][0], inversion[3][1], inversion[3][2]);

	this->ferris_wheel_shader->Use();
	for (int j = 0; j < this->car->meshes.size(); j++)
	{

		glUniform1i(glGetUniformLocation(this->ferris_wheel_shader->Program, "u_texture"), 0);
		switch (color) {
		case RED:
			this->car_red->bind(0);
			break;
		case ORANGE:
			this->car_orange->bind(0);
			break;
		case YELLOW:
			this->car_yellow->bind(0);
			break;
		case GREEN:
			this->car_green->bind(0);
			break;
		case BLUE:
			this->car_blue->bind(0);
			break;
		case BLUE2:
			this->car_blue2->bind(0);
			break;
		case PURPLE:
			this->car_purple->bind(0);
			break;
		case PINK:
			this->car_pink->bind(0);
			break;
		}
		glUniformMatrix4fv(
			glGetUniformLocation(this->ferris_wheel_shader->Program, "model"), 1, GL_FALSE, &model_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->ferris_wheel_shader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->ferris_wheel_shader->Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);

		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "dirLight.direction"), 0, -1.0f, 0);
		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "dirLight.ambient"), 0.1, 0.1, 0.1);
		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "dirLight.diffuse"), 0.5, 0.5, 0.5);
		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "dirLight.specular"), 0.5, 0.5, 0.5);

		glUniform3f(glGetUniformLocation(this->ferris_wheel_shader->Program, "viewPos"), viewerPos.x, viewerPos.y, viewerPos.z);

		// draw mesh
		glBindVertexArray(this->car->meshes[j].VAO);
		glDrawElements(GL_TRIANGLES, this->car->meshes[j].indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}
}

void TrainView::drawWaterSlide()
{
	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, glm::vec3(-70, 35, 100));
	model_matrix = glm::scale(model_matrix, glm::vec3(3, 3, 3));

	glm::mat4 view_matrix, project_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);

	glm::mat4 view;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view[0][0]);
	glm::mat4 inversion = glm::inverse(view);
	glm::vec3 viewerPos(inversion[3][0], inversion[3][1], inversion[3][2]);

	this->water_slide_shader->Use();
	for (int j = 0; j < this->water_slide->meshes.size(); j++)
	{
		for (unsigned int i = 0; i < this->water_slide->meshes[j].textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
			glUniform1i(glGetUniformLocation(this->water_slide_shader->Program, "u_texture"), i);
			//shader.setFloat(("material." + name + number).c_str(), i);
			glBindTexture(GL_TEXTURE_2D, this->water_slide->meshes[j].textures[i].id);
		}
		glUniformMatrix4fv(
			glGetUniformLocation(this->water_slide_shader->Program, "model"), 1, GL_FALSE, &model_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->water_slide_shader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->water_slide_shader->Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);

		glUniform3f(glGetUniformLocation(this->water_slide_shader->Program, "dirLight.direction"), 0, -1.0f, 0);
		glUniform3f(glGetUniformLocation(this->water_slide_shader->Program, "dirLight.ambient"), 0.1, 0.1, 0.1);
		glUniform3f(glGetUniformLocation(this->water_slide_shader->Program, "dirLight.diffuse"), 0.2, 0.2, 0.2);
		glUniform3f(glGetUniformLocation(this->water_slide_shader->Program, "dirLight.specular"), 0.0, 0.0, 0.0);

		glUniform3f(glGetUniformLocation(this->water_slide_shader->Program, "viewPos"), viewerPos.x, viewerPos.y, viewerPos.z);
		glActiveTexture(GL_TEXTURE0);

		// draw mesh
		glBindVertexArray(this->water_slide->meshes[j].VAO);
		glDrawElements(GL_TRIANGLES, this->water_slide->meshes[j].indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}
}

void TrainView::drawWater()
{
	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, glm::vec3(-70, 35, 100));
	model_matrix = glm::scale(model_matrix, glm::vec3(3, 3, 3));

	glm::mat4 view_matrix, project_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);

	glm::mat4 view;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view[0][0]);
	glm::mat4 inversion = glm::inverse(view);
	glm::vec3 viewerPos(inversion[3][0], inversion[3][1], inversion[3][2]);

	this->water_shader->Use();
	for (int j = 0; j < this->water->meshes.size(); j++)
	{
		for (unsigned int i = 0; i < this->water->meshes[j].textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
			glUniform1i(glGetUniformLocation(this->water_shader->Program, "u_texture"), i);
			glBindTexture(GL_TEXTURE_2D, this->water->meshes[j].textures[i].id);
		}
		glUniform1i(glGetUniformLocation(this->water_shader->Program, "heightMap"), 1);
		glUniform1i(glGetUniformLocation(this->water_shader->Program, "u_heightMap"), 2);
		this->height_map[this->count_height_map]->bind(1);
		this->height_map[this->count_height_map]->bind(2);

		glUniformMatrix4fv(
			glGetUniformLocation(this->water_shader->Program, "model"), 1, GL_FALSE, &model_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->water_shader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->water_shader->Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);

		glUniform3f(glGetUniformLocation(this->water_shader->Program, "dirLight.direction"), 0, -1.0f, 0);
		glUniform3f(glGetUniformLocation(this->water_shader->Program, "dirLight.ambient"), 0.1, 0.1, 0.1);
		glUniform3f(glGetUniformLocation(this->water_shader->Program, "dirLight.diffuse"), 0.5, 0.5, 0.5);
		glUniform3f(glGetUniformLocation(this->water_shader->Program, "dirLight.specular"), 1.0, 1.0, 1.0);

		glUniform3f(glGetUniformLocation(this->water_shader->Program, "viewPos"), viewerPos.x, viewerPos.y, viewerPos.z);
		glActiveTexture(GL_TEXTURE0);

		// draw mesh
		glBindVertexArray(this->water->meshes[j].VAO);
		glDrawElements(GL_TRIANGLES, this->water->meshes[j].indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}
}

void TrainView::drawDropTower()
{

	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, glm::vec3(-80, 15, -30));
	model_matrix = glm::scale(model_matrix, glm::vec3(3,3,3));

	glm::mat4 view_matrix, project_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);

	glm::mat4 view;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view[0][0]);
	glm::mat4 inversion = glm::inverse(view);
	glm::vec3 viewerPos(inversion[3][0], inversion[3][1], inversion[3][2]);

	this->drop_tower_shader->Use();
	for (int j = 0; j < this->drop_tower->meshes.size(); j++)
	{
		for (unsigned int i = 0; i < this->drop_tower->meshes[j].textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
			glUniform1i(glGetUniformLocation(this->drop_tower_shader->Program, "u_texture"), i);
			glBindTexture(GL_TEXTURE_2D, this->drop_tower->meshes[j].textures[i].id);
		}
		glUniformMatrix4fv(
			glGetUniformLocation(this->drop_tower_shader->Program, "model"), 1, GL_FALSE, &model_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->drop_tower_shader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->drop_tower_shader->Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);

		glUniform3f(glGetUniformLocation(this->drop_tower_shader->Program, "dirLight.direction"), 0, -1.0f, 0);
		glUniform3f(glGetUniformLocation(this->drop_tower_shader->Program, "dirLight.ambient"), 0.1, 0.1, 0.1);
		glUniform3f(glGetUniformLocation(this->drop_tower_shader->Program, "dirLight.diffuse"), 0.5, 0.5, 0.5);
		glUniform3f(glGetUniformLocation(this->drop_tower_shader->Program, "dirLight.specular"), 0.3, 0.3, 0.3);

		glUniform3f(glGetUniformLocation(this->drop_tower_shader->Program, "viewPos"), viewerPos.x, viewerPos.y, viewerPos.z);
		glActiveTexture(GL_TEXTURE0);

		// draw mesh
		glBindVertexArray(this->drop_tower->meshes[j].VAO);
		glDrawElements(GL_TRIANGLES, this->drop_tower->meshes[j].indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}
}

void TrainView::drawDropTowerSeat()
{
	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, glm::vec3(-80, 15, -30));
	model_matrix = glm::scale(model_matrix, glm::vec3(3, 3, 3));

	glm::mat4 view_matrix, project_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);

	glm::mat4 view;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view[0][0]);
	glm::mat4 inversion = glm::inverse(view);
	glm::vec3 viewerPos(inversion[3][0], inversion[3][1], inversion[3][2]);

	this->drop_tower_shader->Use();
	for (int j = 0; j < this->drop_tower_seat->meshes.size(); j++)
	{
		for (unsigned int i = 0; i < this->drop_tower_seat->meshes[j].textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
			glUniform1i(glGetUniformLocation(this->drop_tower_shader->Program, "u_texture"), i);
			glBindTexture(GL_TEXTURE_2D, this->drop_tower_seat->meshes[j].textures[i].id);
		}
		glUniformMatrix4fv(
			glGetUniformLocation(this->drop_tower_shader->Program, "model"), 1, GL_FALSE, &model_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->drop_tower_shader->Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(this->drop_tower_shader->Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);

		glUniform3f(glGetUniformLocation(this->drop_tower_shader->Program, "dirLight.direction"), 0, -1.0f, 0);
		glUniform3f(glGetUniformLocation(this->drop_tower_shader->Program, "dirLight.ambient"), 0.1, 0.1, 0.1);
		glUniform3f(glGetUniformLocation(this->drop_tower_shader->Program, "dirLight.diffuse"), 0.5, 0.5, 0.5);
		glUniform3f(glGetUniformLocation(this->drop_tower_shader->Program, "dirLight.specular"), 0.3, 0.3, 0.3);

		glUniform3f(glGetUniformLocation(this->drop_tower_shader->Program, "viewPos"), viewerPos.x, viewerPos.y, viewerPos.z);
		glActiveTexture(GL_TEXTURE0);

		// draw mesh
		glBindVertexArray(this->drop_tower_seat->meshes[j].VAO);
		glDrawElements(GL_TRIANGLES, this->drop_tower_seat->meshes[j].indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}
}

void TrainView::drawTrack(TrainView*)
{
	float percent = 1.0f / DIVIDE_LINE;
	const float railWidth = 2.5f;
	this->arc_length.clear();
	this->arc_length.push_back(0);
	this->totalArc = 0;
	for (size_t i = 0; i < m_pTrack->points.size(); i++) {
		// pos
		ControlPoint p0 = m_pTrack->points[i % m_pTrack->points.size()];
		ControlPoint p1 = m_pTrack->points[(i + 1) % m_pTrack->points.size()];
		ControlPoint p2 = m_pTrack->points[(i + 2) % m_pTrack->points.size()];
		ControlPoint p3 = m_pTrack->points[(i + 3) % m_pTrack->points.size()];
		// orient
		float t = percent;
		float countArcLength = 0.0f;
		Pnt3f preQt = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), 0);
		Pnt3f prePreQt;
		for (size_t j = 0; j < DIVIDE_LINE; j++) {
			Pnt3f qt = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), t);
			Pnt3f ori = GMT(p0.orient, p1.orient, p2.orient, p3.orient, tw->splineBrowser->value(), t);

			ori.normalize();
			Pnt3f cross_t = (qt - preQt) * ori;
			cross_t.normalize();
			cross_t = cross_t * railWidth;
			Pnt3f forward = (qt - preQt);
			forward.normalize();
			Pnt3f up = cross_t * forward;

			glLineWidth(4);
			glBegin(GL_LINES);
			glColor3ub(255, 255, 255);
			glVertex3f(preQt.x + cross_t.x, preQt.y + cross_t.y, preQt.z + cross_t.z);
			glVertex3f(qt.x + cross_t.x, qt.y + cross_t.y, qt.z + cross_t.z);
			glVertex3f(preQt.x - cross_t.x, preQt.y - cross_t.y, preQt.z - cross_t.z);
			glVertex3f(qt.x - cross_t.x, qt.y - cross_t.y, qt.z - cross_t.z);
			glVertex3f(preQt.x + cross_t.x - up.x, preQt.y + cross_t.y - up.y, preQt.z + cross_t.z - up.z);
			glVertex3f(qt.x + cross_t.x - up.x, qt.y + cross_t.y - up.y, qt.z + cross_t.z - up.z);
			glVertex3f(preQt.x - cross_t.x - up.x, preQt.y - cross_t.y - up.y, preQt.z - cross_t.z - up.z);
			glVertex3f(qt.x - cross_t.x - up.x, qt.y - cross_t.y - up.y, qt.z - cross_t.z - up.z);
			if (j != 0) //fill gap
			{
				glVertex3f(prePreQt.x - cross_t.x - up.x, prePreQt.y - cross_t.y - up.y, prePreQt.z - cross_t.z - up.z);
				glVertex3f(preQt.x - cross_t.x - up.x, preQt.y - cross_t.y - up.y, preQt.z - cross_t.z - up.z);
				glVertex3f(prePreQt.x - cross_t.x, prePreQt.y - cross_t.y, prePreQt.z - cross_t.z);
				glVertex3f(preQt.x - cross_t.x, preQt.y - cross_t.y, preQt.z - cross_t.z);
			}
			glEnd();
			this->arc_length.push_back((qt - preQt).getLength());
			this->totalArc += (qt - preQt).getLength();
			prePreQt = preQt;
			preQt = qt;
			t += percent;
		}
	}
}

void TrainView::drawTiles()
{
	double accumulate = 3.75;
	double t = 0;
	int i = 0;
	bool ifDraw = true;
	ControlPoint p0 = m_pTrack->points[i % m_pTrack->points.size()];
	ControlPoint p1 = m_pTrack->points[(i + 1) % m_pTrack->points.size()];
	ControlPoint p2 = m_pTrack->points[(i + 2) % m_pTrack->points.size()];
	ControlPoint p3 = m_pTrack->points[(i + 3) % m_pTrack->points.size()];
	Pnt3f preQt = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), 0);
	while (accumulate <= this->totalArc)
	{
		double tmp = 0;
		int count = 0;
		while (accumulate > tmp)
		{
			tmp += this->arc_length[count];
			count++;
		}
		if (tmp == accumulate)
		{
			i = (count - 1) / 20;
			t = ((count - 1) % 20) * 0.05;
		}
		else
		{
			float x = (accumulate - (tmp - this->arc_length[count - 1])) / ((tmp - accumulate) + (accumulate - (tmp - this->arc_length[count - 1])));
			i = (count - 1) / 20;
			t = ((count - 1) % 20 + x) * 0.05;
		}

		ControlPoint p0 = m_pTrack->points[i % m_pTrack->points.size()];
		ControlPoint p1 = m_pTrack->points[(i + 1) % m_pTrack->points.size()];
		ControlPoint p2 = m_pTrack->points[(i + 2) % m_pTrack->points.size()];
		ControlPoint p3 = m_pTrack->points[(i + 3) % m_pTrack->points.size()];
		Pnt3f qt = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), t);
		Pnt3f ori = GMT(p0.orient, p1.orient, p2.orient, p3.orient, tw->splineBrowser->value(), t);

		ori.normalize();
		Pnt3f cross_t = (qt - preQt) * ori;
		cross_t.normalize();
		cross_t = cross_t * 2.5f;
		Pnt3f forward = qt - preQt;
		forward.normalize();
		Pnt3f up = cross_t * forward;
		up.normalize();

		if (ifDraw)
		{
			glColor3ub(255, 170, 249);
				qt = qt - up;
				glBegin(GL_QUADS); //up
				glTexCoord2f(0.0f, 0.0f);
				glVertex3f(qt.x - 1.5 * cross_t.x + forward.x + up.x, qt.y - 1.5 * cross_t.y + forward.y + up.y, qt.z - 1.5 * cross_t.z + forward.z + up.z);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(qt.x + 1.5 * cross_t.x + forward.x + up.x, qt.y + 1.5 * cross_t.y + forward.y + up.y, qt.z + 1.5 * cross_t.z + forward.z + up.z);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(qt.x + 1.5 * cross_t.x - forward.x + up.x, qt.y + 1.5 * cross_t.y - forward.y + up.y, qt.z + 1.5 * cross_t.z - forward.z + up.z);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(qt.x - 1.5 * cross_t.x - forward.x + up.x, qt.y - 1.5 * cross_t.y - forward.y + up.y, qt.z - 1.5 * cross_t.z - forward.z + up.z);
				glEnd();

				glBegin(GL_QUADS); //down
				glTexCoord2f(0.0f, 0.0f);
				glVertex3f(qt.x - 1.5 * cross_t.x + forward.x - up.x, qt.y - 1.5 * cross_t.y + forward.y - up.y, qt.z - 1.5 * cross_t.z + forward.z - up.z);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(qt.x + 1.5 * cross_t.x + forward.x - up.x, qt.y + 1.5 * cross_t.y + forward.y - up.y, qt.z + 1.5 * cross_t.z + forward.z - up.z);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(qt.x + 1.5 * cross_t.x - forward.x - up.x, qt.y + 1.5 * cross_t.y - forward.y - up.y, qt.z + 1.5 * cross_t.z - forward.z - up.z);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(qt.x - 1.5 * cross_t.x - forward.x - up.x, qt.y - 1.5 * cross_t.y - forward.y - up.y, qt.z - 1.5 * cross_t.z - forward.z - up.z);
				glEnd();

				glBegin(GL_QUADS); //front
				glTexCoord2f(0.0f, 0.0f);
				glVertex3f(qt.x - 1.5 * cross_t.x + forward.x + up.x, qt.y - 1.5 * cross_t.y + forward.y + up.y, qt.z - 1.5 * cross_t.z + forward.z + up.z);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(qt.x + 1.5 * cross_t.x + forward.x + up.x, qt.y + 1.5 * cross_t.y + forward.y + up.y, qt.z + 1.5 * cross_t.z + forward.z + up.z);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(qt.x + 1.5 * cross_t.x + forward.x - up.x, qt.y + 1.5 * cross_t.y + forward.y - up.y, qt.z + 1.5 * cross_t.z + forward.z - up.z);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(qt.x - 1.5 * cross_t.x + forward.x - up.x, qt.y - 1.5 * cross_t.y + forward.y - up.y, qt.z - 1.5 * cross_t.z + forward.z - up.z);
				glEnd();

				glBegin(GL_QUADS); //back
				glTexCoord2f(0.0f, 0.0f);
				glVertex3f(qt.x - 1.5 * cross_t.x - forward.x + up.x, qt.y - 1.5 * cross_t.y - forward.y + up.y, qt.z - 1.5 * cross_t.z - forward.z + up.z);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(qt.x + 1.5 * cross_t.x - forward.x + up.x, qt.y + 1.5 * cross_t.y - forward.y + up.y, qt.z + 1.5 * cross_t.z - forward.z + up.z);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(qt.x + 1.5 * cross_t.x - forward.x - up.x, qt.y + 1.5 * cross_t.y - forward.y - up.y, qt.z + 1.5 * cross_t.z - forward.z - up.z);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(qt.x - 1.5 * cross_t.x - forward.x - up.x, qt.y - 1.5 * cross_t.y - forward.y - up.y, qt.z - 1.5 * cross_t.z - forward.z - up.z);
				glEnd();

				glBegin(GL_QUADS); //left
				glTexCoord2f(0.0f, 0.0f);
				glVertex3f(qt.x - 1.5 * cross_t.x + forward.x + up.x, qt.y - 1.5 * cross_t.y + forward.y + up.y, qt.z - 1.5 * cross_t.z + forward.z + up.z);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(qt.x - 1.5 * cross_t.x + forward.x - up.x, qt.y - 1.5 * cross_t.y + forward.y - up.y, qt.z - 1.5 * cross_t.z + forward.z - up.z);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(qt.x - 1.5 * cross_t.x - forward.x - up.x, qt.y - 1.5 * cross_t.y - forward.y - up.y, qt.z - 1.5 * cross_t.z - forward.z - up.z);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(qt.x - 1.5 * cross_t.x - forward.x + up.x, qt.y - 1.5 * cross_t.y - forward.y + up.y, qt.z - 1.5 * cross_t.z - forward.z + up.z);
				glEnd();

				glBegin(GL_QUADS); //right
				glTexCoord2f(0.0f, 0.0f);
				glVertex3f(qt.x + 1.5 * cross_t.x + forward.x + up.x, qt.y + 1.5 * cross_t.y + forward.y + up.y, qt.z + 1.5 * cross_t.z + forward.z + up.z);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(qt.x + 1.5 * cross_t.x + forward.x - up.x, qt.y + 1.5 * cross_t.y + forward.y - up.y, qt.z + 1.5 * cross_t.z + forward.z - up.z);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(qt.x + 1.5 * cross_t.x - forward.x - up.x, qt.y + 1.5 * cross_t.y - forward.y - up.y, qt.z + 1.5 * cross_t.z - forward.z - up.z);
				glTexCoord2f(0.0f, 1.0f);
				glVertex3f(qt.x + 1.5 * cross_t.x - forward.x + up.x, qt.y + 1.5 * cross_t.y - forward.y + up.y, qt.z + 1.5 * cross_t.z - forward.z + up.z);
				glEnd();
		}
		ifDraw = !ifDraw;
		accumulate += 3.75;
		preQt = qt;
	}
}

void TrainView::drawTrain(TrainView*)
{
	int color[7][3] = {
		{255, 0, 0},
		{ 255, 165, 0},
		{255, 255, 0},
		{0, 255, 0},
		{0, 127, 255},
		{0, 0, 255 },
		{139, 0, 255 }
	};
	int i = 0;
	float t = 0;
	double percent = 1.0f / DIVIDE_LINE;
	double tmp = 0;
	int count = 0;
	if (this->trainU > this->totalArc)
	{
		this->trainU -= this->totalArc;
	}
	Pnt3f preQt;
	int carNum = 8;
	for (int j = 0; j < carNum; j++)
	{
		double tmp = 0;
		int count = 0;
		double pos = this->trainU - 20 * j;
		if (pos < 0)
		{
			pos += this->totalArc;
		}
		while (pos > tmp)
		{
			tmp += this->arc_length[count];
			count++;
		}
		if (tmp == pos)
		{
			i = (count - 1) / 20;
			t = ((count - 1) % 20) * 0.05;
		}
		else
		{
			float x = (pos - (tmp - this->arc_length[count - 1])) / ((tmp - pos) + (pos - (tmp - this->arc_length[count - 1])));
			i = (count - 1) / 20;
			t = ((count - 1) % 20 + x) * 0.05;
		}
		ControlPoint p0 = m_pTrack->points[i % m_pTrack->points.size()];
		ControlPoint p1 = m_pTrack->points[(i + 1) % m_pTrack->points.size()];
		ControlPoint p2 = m_pTrack->points[(i + 2) % m_pTrack->points.size()];
		ControlPoint p3 = m_pTrack->points[(i + 3) % m_pTrack->points.size()];
		Pnt3f qt = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), t);
		Pnt3f nextQt = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), t + percent);
		Pnt3f ori = GMT(p0.orient, p1.orient, p2.orient, p3.orient, tw->splineBrowser->value(), t);
		Pnt3f forward = nextQt - qt;
		forward.normalize();
		Pnt3f cross_t = forward * ori;
		cross_t.normalize();
		Pnt3f up = cross_t * forward;
		up.normalize();
		if (j == 0)
		{
			if (forward.y > 0)
			{
				this->trainAcc += forward.y * tw->speed->value() * -0.2;

			}
			else if (forward.y < 0)
			{
				this->trainAcc += forward.y * tw->speed->value() * -0.1;
			}
			if (this->trainAcc <= 0.7 * tw->speed->value() * -1)
			{
				this->trainAcc = 0.7 * tw->speed->value() * -1;
			}
		}
		cross_t = cross_t * 5;
		forward = forward * 5;
		up = up * 5;
		qt = qt + 1.5 * up;
		int train_len = 2;
		float wheel_len = 0.3;

		if (j == 0)
		{
			glColor3ub(166, 12, 0);
			glBegin(GL_QUADS); //back
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(qt.x - forward.x * train_len - cross_t.x - up.x, qt.y - forward.y * train_len - cross_t.y - up.y, qt.z - forward.z * train_len - cross_t.z - up.z);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(qt.x - forward.x * train_len - cross_t.x, qt.y - forward.y * train_len - cross_t.y, qt.z - forward.z * train_len - cross_t.z);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(qt.x - forward.x * train_len + cross_t.x, qt.y - forward.y * train_len + cross_t.y, qt.z - forward.z * train_len + cross_t.z);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(qt.x - forward.x * train_len + cross_t.x - up.x, qt.y - forward.y * train_len + cross_t.y - up.y, qt.z - forward.z * train_len + cross_t.z - up.z);
			glEnd();

			glColor3ub(0, 0, 0);
			glBegin(GL_QUADS); //left quad
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(qt.x - cross_t.x, qt.y - cross_t.y, qt.z - cross_t.z);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(qt.x - cross_t.x - up.x, qt.y - cross_t.y - up.y, qt.z - cross_t.z - up.z);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(qt.x - forward.x * train_len - cross_t.x - up.x, qt.y - forward.y * train_len - cross_t.y - up.y, qt.z - forward.z * train_len - cross_t.z - up.z);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(qt.x - forward.x * train_len - cross_t.x, qt.y - forward.y * train_len - cross_t.y, qt.z - forward.z * train_len - cross_t.z);
			glEnd();

			glBegin(GL_QUADS); //right quad
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(qt.x + cross_t.x, qt.y + cross_t.y, qt.z + cross_t.z);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(qt.x + cross_t.x - up.x, qt.y + cross_t.y - up.y, qt.z + cross_t.z - up.z);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(qt.x - forward.x * train_len + cross_t.x - up.x, qt.y - forward.y * train_len + cross_t.y - up.y, qt.z - forward.z * train_len + cross_t.z - up.z);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(qt.x - forward.x * train_len + cross_t.x, qt.y - forward.y * train_len + cross_t.y, qt.z - forward.z * train_len + cross_t.z);
			glEnd();

			glColor3ub(255, 255, 0);
			glBegin(GL_TRIANGLES); //left triangle
			glVertex3f(qt.x - cross_t.x + up.x, qt.y - cross_t.y + up.y, qt.z - cross_t.z + up.z);
			glVertex3f(qt.x + forward.x * train_len - cross_t.x - up.x, qt.y + forward.y * train_len - cross_t.y - up.y, qt.z + forward.z * train_len - cross_t.z - up.z);
			glVertex3f(qt.x - cross_t.x - up.x, qt.y - cross_t.y - up.y, qt.z - cross_t.z - up.z);
			glEnd();

			glBegin(GL_TRIANGLES); //right triangle
			glVertex3f(qt.x + cross_t.x + up.x, qt.y + cross_t.y + up.y, qt.z + cross_t.z + up.z);
			glVertex3f(qt.x + forward.x * train_len + cross_t.x - up.x, qt.y + forward.y * train_len + cross_t.y - up.y, qt.z + forward.z * train_len + cross_t.z - up.z);
			glVertex3f(qt.x + cross_t.x - up.x, qt.y + cross_t.y - up.y, qt.z + cross_t.z - up.z);
			glEnd();


			glColor3ub(0, 0, 0);
			glBegin(GL_TRIANGLES); //left triangle inside
			glVertex3f(qt.x - 0.95 * cross_t.x + up.x, qt.y - 0.95 * cross_t.y + up.y, qt.z - 0.95 * cross_t.z + up.z);
			glVertex3f(qt.x + forward.x * train_len - 0.95 * cross_t.x - up.x, qt.y + forward.y * train_len - 0.95 * cross_t.y - up.y, qt.z + forward.z * train_len - 0.95 * cross_t.z - up.z);
			glVertex3f(qt.x - 0.95 * cross_t.x - up.x, qt.y - 0.95 * cross_t.y - up.y, qt.z - 0.95 * cross_t.z - up.z);
			glEnd();

			glBegin(GL_TRIANGLES); //right triangle inside
			glVertex3f(qt.x + 0.95 * cross_t.x + up.x, qt.y + 0.95 * cross_t.y + up.y, qt.z + 0.95 * cross_t.z + up.z);
			glVertex3f(qt.x + forward.x * train_len + 0.95 * cross_t.x - up.x, qt.y + forward.y * train_len + 0.95 * cross_t.y - up.y, qt.z + forward.z * train_len + 0.95 * cross_t.z - up.z);
			glVertex3f(qt.x + 0.95 * cross_t.x - up.x, qt.y + 0.95 * cross_t.y - up.y, qt.z + 0.95 * cross_t.z - up.z);
			glEnd();

			glBegin(GL_QUADS); //down
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(qt.x + forward.x * train_len - cross_t.x - up.x, qt.y + forward.y * train_len - cross_t.y - up.y, qt.z + forward.z * train_len - cross_t.z - up.z);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(qt.x + forward.x * train_len + cross_t.x - up.x, qt.y + forward.y * train_len + cross_t.y - up.y, qt.z + forward.z * train_len + cross_t.z - up.z);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(qt.x - forward.x * train_len + cross_t.x - up.x, qt.y - forward.y * train_len + cross_t.y - up.y, qt.z - forward.z * train_len + cross_t.z - up.z);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(qt.x - forward.x * train_len - cross_t.x - up.x, qt.y - forward.y * train_len - cross_t.y - up.y, qt.z - forward.z * train_len - cross_t.z - up.z);
			glEnd();


			glColor3ub(0, 0, 0);
			glBegin(GL_QUADS); //top
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(qt.x + -cross_t.x + up.x, qt.y - cross_t.y + up.y, qt.z - cross_t.z + up.z);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(qt.x + cross_t.x + up.x, qt.y + cross_t.y + up.y, qt.z + cross_t.z + up.z);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(qt.x + forward.x * train_len + cross_t.x - up.x, qt.y + forward.y * train_len + cross_t.y - up.y, qt.z + forward.z * train_len + cross_t.z - up.z);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(qt.x + forward.x * train_len - cross_t.x - up.x, qt.y + forward.y * train_len - cross_t.y - up.y, qt.z + forward.z * train_len - cross_t.z - up.z);
			glEnd();

		}
		else
		{
			forward = forward * 0.5;

			glBegin(GL_LINES); //connect cable
			glLineWidth(2);

			glColor3ub(255, 255, 255);
			glVertex3f(qt.x + forward.x * train_len - 0.5 * up.x, qt.y + forward.y * train_len - 0.5 * up.y, qt.z + forward.z * train_len - 0.5 * up.z);
			glVertex3f(preQt.x, preQt.y, preQt.z);
			glEnd();


			glColor3ub(color[(j - 1) % 7][0], color[(j - 1) % 7][1], color[(j - 1) % 7][2]);
			glBegin(GL_QUADS); //back
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(qt.x - forward.x * train_len - cross_t.x - up.x, qt.y - forward.y * train_len - cross_t.y - up.y, qt.z - forward.z * train_len - cross_t.z - up.z);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(qt.x - forward.x * train_len - cross_t.x, qt.y - forward.y * train_len - cross_t.y, qt.z - forward.z * train_len - cross_t.z);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(qt.x - forward.x * train_len + cross_t.x, qt.y - forward.y * train_len + cross_t.y, qt.z - forward.z * train_len + cross_t.z);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(qt.x - forward.x * train_len + cross_t.x - up.x, qt.y - forward.y * train_len + cross_t.y - up.y, qt.z - forward.z * train_len + cross_t.z - up.z);
			glEnd();

			glBegin(GL_QUADS); //front
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(qt.x + forward.x * train_len - cross_t.x - up.x, qt.y + forward.y * train_len - cross_t.y - up.y, qt.z + forward.z * train_len - cross_t.z - up.z);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(qt.x + forward.x * train_len - cross_t.x, qt.y + forward.y * train_len - cross_t.y, qt.z + forward.z * train_len - cross_t.z);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(qt.x + forward.x * train_len + cross_t.x, qt.y + forward.y * train_len + cross_t.y, qt.z + forward.z * train_len + cross_t.z);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(qt.x + forward.x * train_len + cross_t.x - up.x, qt.y + forward.y * train_len + cross_t.y - up.y, qt.z + forward.z * train_len + cross_t.z - up.z);
			glEnd();

			glBegin(GL_QUADS); //left
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(qt.x + forward.x * train_len - cross_t.x, qt.y + forward.y * train_len - cross_t.y, qt.z + forward.z * train_len - cross_t.z);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(qt.x - forward.x * train_len - cross_t.x, qt.y - forward.y * train_len - cross_t.y, qt.z - forward.z * train_len - cross_t.z);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(qt.x - forward.x * train_len - cross_t.x - up.x, qt.y - forward.y * train_len - cross_t.y - up.y, qt.z - forward.z * train_len - cross_t.z - up.z);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(qt.x + forward.x * train_len - cross_t.x - up.x, qt.y + forward.y * train_len - cross_t.y - up.y, qt.z + forward.z * train_len - cross_t.z - up.z);
			glEnd();


			glBegin(GL_QUADS); //right
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(qt.x + forward.x * train_len + cross_t.x, qt.y + forward.y * train_len + cross_t.y, qt.z + forward.z * train_len + cross_t.z);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(qt.x - forward.x * train_len + cross_t.x, qt.y - forward.y * train_len + cross_t.y, qt.z - forward.z * train_len + cross_t.z);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(qt.x - forward.x * train_len + cross_t.x - up.x, qt.y - forward.y * train_len + cross_t.y - up.y, qt.z - forward.z * train_len + cross_t.z - up.z);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(qt.x + forward.x * train_len + cross_t.x - up.x, qt.y + forward.y * train_len + cross_t.y - up.y, qt.z + forward.z * train_len + cross_t.z - up.z);
			glEnd();

			glColor3ub(0, 0, 0);
			glBegin(GL_QUADS); //down
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(qt.x + forward.x * train_len - cross_t.x - up.x, qt.y + forward.y * train_len - cross_t.y - up.y, qt.z + forward.z * train_len - cross_t.z - up.z);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(qt.x + forward.x * train_len + cross_t.x - up.x, qt.y + forward.y * train_len + cross_t.y - up.y, qt.z + forward.z * train_len + cross_t.z - up.z);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(qt.x - forward.x * train_len + cross_t.x - up.x, qt.y - forward.y * train_len + cross_t.y - up.y, qt.z - forward.z * train_len + cross_t.z - up.z);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(qt.x - forward.x * train_len - cross_t.x - up.x, qt.y - forward.y * train_len - cross_t.y - up.y, qt.z - forward.z * train_len - cross_t.z - up.z);
			glEnd();

		}

		glBegin(GL_TRIANGLE_FAN); //left front wheel  
		glColor3f(255, 255, 255);
		glVertex3f(qt.x + 0.5 * forward.x * train_len - 0.5 * cross_t.x - 1.2 * up.x, qt.y + 0.5 * forward.y * train_len - 0.5 * cross_t.y - 1.2 * up.y, qt.z + 0.5 * forward.z * train_len - 0.5 * cross_t.z - 1.2 * up.z);
		int i = 0;
		for (i = 0; i <= 360; i += 15)
		{
			float p = i * 3.14 / 180;
			glColor3f(0, 0, 0);
			glVertex3f(qt.x + 0.5 * forward.x * train_len - 0.5 * cross_t.x - 1.2 * up.x + sin(p) * -wheel_len * forward.x + up.x * cos(p) * wheel_len, qt.y + 0.5 * forward.y * train_len - 0.5 * cross_t.y - 1.2 * up.y + sin(p) * -wheel_len * forward.y + up.y * cos(p) * wheel_len, qt.z + 0.5 * forward.z * train_len - 0.5 * cross_t.z - 1.2 * up.z + sin(p) * -wheel_len * forward.z + up.z * cos(p) * wheel_len);
		}
		glEnd();

		glBegin(GL_TRIANGLE_FAN); //left back wheel  
		glColor3f(255, 255, 255);
		glVertex3f(qt.x - 0.5 * forward.x * train_len - 0.5 * cross_t.x - 1.2 * up.x, qt.y - 0.5 * forward.y * train_len - 0.5 * cross_t.y - 1.2 * up.y, qt.z - 0.5 * forward.z * train_len - 0.5 * cross_t.z - 1.2 * up.z);
		for (i = 0; i <= 360; i += 15)
		{
			float p = i * 3.14 / 180;
			glColor3f(0, 0, 0);
			glVertex3f(qt.x - 0.5 * forward.x * train_len - 0.5 * cross_t.x - 1.2 * up.x + sin(p) * -wheel_len * forward.x + up.x * cos(p) * wheel_len, qt.y - 0.5 * forward.y * train_len - 0.5 * cross_t.y - 1.2 * up.y + sin(p) * -wheel_len * forward.y + up.y * cos(p) * wheel_len, qt.z - 0.5 * forward.z * train_len - 0.5 * cross_t.z - 1.2 * up.z + sin(p) * -wheel_len * forward.z + up.z * cos(p) * wheel_len);
		}
		glEnd();

		glBegin(GL_TRIANGLE_FAN); //right back wheel  
		glColor3f(255, 255, 255);
		glVertex3f(qt.x - 0.5 * forward.x * train_len + 0.5 * cross_t.x - 1.2 * up.x, qt.y - 0.5 * forward.y * train_len + 0.5 * cross_t.y - 1.2 * up.y, qt.z - 0.5 * forward.z * train_len + 0.5 * cross_t.z - 1.2 * up.z);
		for (i = 0; i <= 360; i += 15)
		{
			float p = i * 3.14 / 180;
			glColor3f(0, 0, 0);
			glVertex3f(qt.x - 0.5 * forward.x * train_len + 0.5 * cross_t.x - 1.2 * up.x + sin(p) * -wheel_len * forward.x + up.x * cos(p) * wheel_len, qt.y - 0.5 * forward.y * train_len + 0.5 * cross_t.y - 1.2 * up.y + sin(p) * -wheel_len * forward.y + up.y * cos(p) * wheel_len, qt.z - 0.5 * forward.z * train_len + 0.5 * cross_t.z - 1.2 * up.z + sin(p) * -wheel_len * forward.z + up.z * cos(p) * wheel_len);
		}
		glEnd();

		glBegin(GL_TRIANGLE_FAN); //right front wheel  
		glColor3f(255, 255, 255);
		glVertex3f(qt.x + 0.5 * forward.x * train_len + 0.5 * cross_t.x - 1.2 * up.x, qt.y + 0.5 * forward.y * train_len + 0.5 * cross_t.y - 1.2 * up.y, qt.z + 0.5 * forward.z * train_len + 0.5 * cross_t.z - 1.2 * up.z);
		for (i = 0; i <= 360; i += 15) {
			float p = i * 3.14 / 180;
			glColor3f(0, 0, 0);
			glVertex3f(qt.x + 0.5 * forward.x * train_len + 0.5 * cross_t.x - 1.2 * up.x + sin(p) * -wheel_len * forward.x + up.x * cos(p) * wheel_len, qt.y + 0.5 * forward.y * train_len + 0.5 * cross_t.y - 1.2 * up.y + sin(p) * -wheel_len * forward.y + up.y * cos(p) * wheel_len, qt.z + 0.5 * forward.z * train_len + 0.5 * cross_t.z - 1.2 * up.z + sin(p) * -wheel_len * forward.z + up.z * cos(p) * wheel_len);
		}
		glEnd();
		preQt = qt - forward * train_len - 0.5 * up;
	}
}

void TrainView::drawSkybox()
{
	glm::mat4 view_matrix = glm::mat4();
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	view_matrix = glm::inverse(view_matrix);
	glm::vec3 pos = { view_matrix[3][0],view_matrix[3][1],view_matrix[3][2] };
	if (tw->stars->value()) {
		this->renderSkyBox(*(this->skybox_shader), pos, this->skybox_stars_sky);
	}
	else if (tw->white->value()) {
		this->renderSkyBox(*(this->skybox_shader), pos, this->skybox_whitesky);
	}
}

Pnt3f TrainView::GMT(const Pnt3f p0, const Pnt3f p1, const Pnt3f p2, const Pnt3f p3, const int type, const float t)
{
	glm::mat4x4 M;

	if (type == 0 || type == 1) //default or linear
	{
		M = {
			0,0,0,0,
			0,0,-1,1,
			0,0,1,0,
			0,0,0,0
		};
	}
	else if (type == 2) //cardinal
	{
		M = {
			-1,2,-1,0,
			3,-5,0,2,
			-3,4,1,0,
			1,-1,0,0
		};
		M /= 2;
	}
	else if (type == 3) //b-spline
	{
		M = {
			-1,3,-3,1,
			3,-6,0,4,
			-3,3,3,1,
			1,0,0,0
		};
		M /= 6.0f;
	}
	M = glm::transpose(M);
	glm::mat4x4 G = {
		p0.x, p0.y, p0.z, 1.0f,
		p1.x, p1.y, p1.z, 1.0f,
		p2.x, p2.y, p2.z, 1.0f,
		p3.x, p3.y, p3.z, 1.0f
	};
	glm::vec4 T = { t * t * t,t * t,t,1.0f };
	glm::vec4 outcome = G * M * T;
	return Pnt3f(outcome[0], outcome[1], outcome[2]);
}

//--------------------------
// 
// Process skybox
// 
//--------------------------

void TrainView::loadSkyBox(GLuint& toBind, vector<string> paths) {

	if (!this->skybox_points) {
		this->skybox_points = new VAO;
		GLfloat skyboxVertices[] = {
			// positions          
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,


			-1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			-1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f
		};

		glGenVertexArrays(1, &this->skybox_points->vao);
		glGenBuffers(1, this->skybox_points->vbo);


		glBindVertexArray(this->skybox_points->vao);

		// Position attribute
		glBindBuffer(GL_ARRAY_BUFFER, this->skybox_points->vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);

		// Unbind VAO
		glBindVertexArray(0);
	}




	/// <summary>
	/// The textures part
	/// </summary>

	glGenTextures(1, &toBind);
	glBindTexture(GL_TEXTURE_CUBE_MAP, toBind);
	//glActiveTexture(GL_TEXTURE_CUBE_MAP);\

	/*const char* all_paths_cube[] = {
		"Images/skybox/right.png","Images/skybox/left.png","Images/skybox/top.png"
		, "Images/skybox/bottom.png", "Images/skybox/back.png","Images/skybox/front.png"
	};*/
	const char* all_paths_cube[] = {
		"Images/skybox/right.jpg","Images/skybox/left.jpg","Images/skybox/top.jpg"
		, "Images/skybox/bottom.jpg", "Images/skybox/back.jpg", "Images/skybox/front.jpg"
	};

	///
	/// r,l,t,b,back,f
	/// <summary>
	/// 				"Images/skybox/right.jpg","Images/skybox/left.jpg","Images/skybox/top.jpg"
	///               , "Images/skybox/bottom.jpg", "Images/skybox/back.jpg", "Images/skybox/front.jpg"
	/// </summary>
	/// 
	if (paths.size() == 0) {

		for (int i = 0; i < 6; ++i) {
			cv::Mat img;
			img = cv::imread(all_paths_cube[i], cv::IMREAD_COLOR);
			//Texture2D tmpCube(all_paths_cube[i]);
			//cout << tmpCube.size.x << " " << tmpCube.size.y << endl;
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, img.cols, img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, img.data);
			img.release();
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	}
	else {

		for (int i = 0; i < 6; ++i)
		{
			cv::Mat img;
			img = cv::imread(paths[i], cv::IMREAD_COLOR);
			//Texture2D tmpCube(all_paths_cube[i]);
			//cout << tmpCube.size.x << " " << tmpCube.size.y << endl;
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, img.cols, img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, img.data);
			img.release();
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);


	}

}

void TrainView::renderSkyBox(Shader& s, glm::vec3 user_position, GLuint& toBind) {
	glDepthMask(GL_FALSE);
	s.Use();
	// ... set view and projection matrix
	glm::mat4 view_matrix = glm::mat4();
	glm::mat4 project_matrix = glm::mat4();
	//glEnable(GL_CULL_FACE);
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);
	view_matrix = glm::translate(view_matrix, user_position);
	view_matrix = glm::scale(view_matrix, glm::vec3(1000.0f, 1000.0f, 1000.0f));

	glUniformMatrix4fv(
		glGetUniformLocation(s.Program, "view"), 1, GL_FALSE, &view_matrix[0][0]);
	glUniformMatrix4fv(
		glGetUniformLocation(s.Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, toBind);
	glUniform1i(glGetUniformLocation(s.Program, "skybox"), 0);

	glBindVertexArray(this->skybox_points->vao);

	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);
	// ... draw rest of the scene

	//unbind VAO
	glBindVertexArray(0);

	//unbind shader(switch to fixed pipeline)
	glUseProgram(0);
}
