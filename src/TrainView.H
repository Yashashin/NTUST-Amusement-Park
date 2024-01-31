/************************************************************************
     File:        TrainView.H

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

#pragma once

#include "RenderUtilities/BufferObject.h"
#include "RenderUtilities/Shader.h"
#include "RenderUtilities/Texture.h"

// Preclarify for preventing the compiler error
class TrainWindow;
class CTrack;


//#######################################################################
// TODO
// You might change the TrainView in order to add different objects to
// be drawn, or adjust the lighting, or ...
//#######################################################################


// The TrainView also needs to handle its user interface events.
// since this is an FLTk Window, we need to include FlTk Headers
// the pragmas turn off the warnings from FlTk - our code should have no warnings,
// but their code doesn't meet that standard
#pragma warning(push)
#pragma warning(disable:4312)
#pragma warning(disable:4311)
#include <Fl/Fl_Gl_Window.h>
#pragma warning(pop)

#include <AL/alut.h>
//#include <AL/al.h>
//#include <AL/alc.h>

#include "Model.h"

// this uses the old ArcBall Code
#include "Utilities/ArcBallCam.H"
#include "Utilities/Pnt3f.H"

#include "../AniModel.h"
#include "../ParticleSystem.h"


class TrainView : public Fl_Gl_Window
{
	public:
		// note that we keep the "standard widget" constructor arguments
		TrainView(int x, int y, int w, int h, const char* l = 0);

		// overrides of important window things
		virtual int handle(int);
		virtual void draw();

		// all of the actual drawing happens in this routine
		// it has to be encapsulated, since we draw differently if
		// we're drawing shadows (no colors, for example)
		void drawStuff(bool doingShadows=false);

		// setup the projection - assuming that the projection stack has been
		// cleared for you
		void setProjection();

		// Reset the Arc ball control
		void resetArcball();

		// pick a point (for when the mouse goes down)
		void doPick();

		//set ubo
		void setUBO();

		//draw cup
		void drawCup(Model* cup);

		void drawCupBase();

		void drawTeapot();

		void drawFerrisWheelMain();

		void drawWheel();

		void drawCar(int color);

		void drawWaterSlide();

		void drawWater();

		void drawDropTower();

		void drawDropTowerSeat();

		void drawTrack(TrainView*);

		void drawTiles();

		void drawTrain(TrainView*);

		void drawSkybox();
		Pnt3f GMT(const Pnt3f p0,const Pnt3f p1,const Pnt3f p2,const Pnt3f p3,const int type,const float t);
		
		void loadSkyBox(GLuint& toBind, vector<string> paths = vector<string>());

		void renderSkyBox(Shader& s, glm::vec3 user_position, GLuint& toBind);



	public:
		ArcBallCam		arcball;			// keep an ArcBall for the UI
		int				selectedCube = -1;  // simple - just remember which cube is selected

		TrainWindow*	tw;				// The parent of this display window
		CTrack*			m_pTrack;		// The track of the entire scene

		Shader* shader		= nullptr;
		Texture2D* texture	= nullptr;
		VAO* plane			= nullptr;
		UBO* commom_matrices= nullptr;

		//Cup
		Model* blue_cup=nullptr;
		Model* red_cup = nullptr;
		Model* yellow_cup = nullptr;
		Model* green_cup = nullptr;
		Shader* cup_shader = nullptr;
		
		//Teapot
		Model* teapot = nullptr;
		Shader* teapot_shader = nullptr;

		//Cup base
		Model* cup_base=nullptr;
		Shader* cup_base_shader = nullptr;

		//Ferris wheel
		Model* ferris_wheel_main = nullptr;
		Model* wheel = nullptr;
		Model* car = nullptr;
		Shader* ferris_wheel_shader = nullptr;
		Texture2D* car_red = nullptr;
		Texture2D* car_orange = nullptr;
		Texture2D* car_yellow = nullptr;
		Texture2D* car_green = nullptr;
		Texture2D* car_blue = nullptr;
		Texture2D* car_blue2 = nullptr;
		Texture2D* car_purple = nullptr;
		Texture2D* car_pink = nullptr;

		//Water slide
		Model* water_slide = nullptr;
		Shader* water_slide_shader = nullptr;
		Model* water = nullptr;
		Shader* water_shader = nullptr;
		Texture2D* height_map[200] = { nullptr };

		//Roller coaster
		vector<double> arc_length;
		double totalArc = 0;
		float trainU=0;
		float trainAcc = 0;
		int countPoint = 0;
		//time
		float time=0.01f;

		int count_height_map = 0;

		float dist = 0.0;


		// SkyBox
		Shader* skybox_shader = nullptr;
		VAO* skybox_points = nullptr;
		GLuint skybox_whitesky;
		GLuint skybox_stars_sky;

		//character
		//For spinning cup model
		Model* cowboy_sit = nullptr;
		Shader* cowboy_sit_shader = nullptr;
		Model* cowboy_sit_handsUp = nullptr;
		Shader* cowboy_sit_shader_handsUp = nullptr;

		//character
		vector<AniModel> animations;
		AniModel* test_ani = nullptr;
		Shader* test_shader_ani = nullptr;

		//floor
		Model* floor = nullptr;
		Shader* floor_shader = nullptr;
	
		// Particle 
		ParticleSystem* psystem = nullptr;
		Shader* particle_shader = nullptr;

		//drop tower
		Model* drop_tower = nullptr;
		Model* drop_tower_seat = nullptr;
		Shader* drop_tower_shader = nullptr;

		//OpenAL
		glm::vec3 source_pos;
		glm::vec3 listener_pos;
		ALCdevice* device	= nullptr;
		ALCcontext* context = nullptr;
		ALuint source;
		ALuint buffer;

};