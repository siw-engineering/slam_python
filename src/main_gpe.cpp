#include <iostream>
#include <pangolin/pangolin.h>
#include <limits>
#include "shaders/Shaders.h"
#include "inputs/ros/DepthSubscriber.h"
#include "inputs/ros/RGBSubscriber.h"
#include "Camera.h"
#include "GPUTexture.h"

using namespace GSLAM;

int main(int argc, char *argv[])
{
	ros::init(argc, argv, "test_node");
	ros::NodeHandle nh;
	DepthSubscriber* depthsub;
	RGBSubscriber* rgbsub;
	cv::Mat dimg, img;
	int width = 640/2;
	int height = 480/2;
	std::shared_ptr<Shader> warper_program, test_program;
	GSLAM::CameraPinhole cam(320,240,277,277,160,120);
	// GPUTexture *rgb, *depth;
	
	depthsub  = new DepthSubscriber("/X1/front/depth", nh);
	rgbsub = new RGBSubscriber("/X1/front/image_raw", nh);

	// rgb = new GPUTexture(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, true, true);
	// depth = new GPUTexture(width, height, GL_LUMINANCE32F_ARB, GL_LUMINANCE, GL_FLOAT);


	/* Display code */
	pangolin::Params windowParams;
	pangolin::CreateWindowAndBind("Main", width, height, windowParams);
	pangolin::OpenGlRenderState s_cam(
		pangolin::ProjectionMatrix(640,480,420,420,320,240,0.1,1000),
		pangolin::ModelViewLookAt(-1,1,-1, 0,0,0, pangolin::AxisY)
	);	
	pangolin::View& d_cam = pangolin::Display("cam")
	.SetBounds(0,1.0f,0,1.0f,-640/480.0)
	.SetHandler(new pangolin::Handler3D(s_cam)
	);
	glEnable(GL_DEPTH_TEST	);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_ALWAYS);
	test_program = std::shared_ptr<Shader>(loadProgramFromFile("test.vert", "test.frag"));
	warper_program = std::shared_ptr<Shader>(loadProgramFromFile("warper.vert"));
  	GLuint vbo, ebo;
    float vertices[] = {
    // positions          // colors           // texture coords
      1,  1, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
      1,  -1, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
     -1,   -1, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
     -1,  1, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left 
};
	int indices[] = {
		0,1,3,
		1,2,3,
	};

  	glGenBuffers(1, &vbo);
  	glBindBuffer(GL_ARRAY_BUFFER, vbo);
  	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  	
  	// glGenBuffers(1, &ebo);
  	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  	// glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);



  	// glGenVertexArrays(1, &vao);
  	// glBindVertexArray(vao);
  	glEnableVertexAttribArray(0);
  	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), 0);
  	
  	// glGenVertexArrays(1, &cao);
  	// glBindVertexArray(cao);
  	glEnableVertexAttribArray(1);
  	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
  	
  	// glGenVertexArrays(1, &tao);
  	// glBindVertexArray(tao);
  	glEnableVertexAttribArray(2);
  	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)) );

	// glBindVertexArray(0);
 //    glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	GLuint rgb_tid, depth_tid;
	void* data;
	data = NULL;

	glGenTextures(1,&rgb_tid);
	glBindTexture(GL_TEXTURE_2D, rgb_tid);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	CheckGlDieOnError();

	glGenTextures(1,&depth_tid);
	glBindTexture(GL_TEXTURE_2D, depth_tid);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	CheckGlDieOnError();


	GLuint fid;
	glGenTransformFeedbacks(1, &fid);
  	// pangolin::GlTexture* imageTexture(width,height,GL_RGB,false,0,GL_RGB,GL_UNSIGNED_BYTE);
	while (ros::ok())
	{

		img  = rgbsub->read();
		dimg = depthsub->read();
		if (dimg.empty() || img.empty())
		{
			ros::spinOnce();
			continue;	
		}
		d_cam.Activate(s_cam);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		/*texture testing*/

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, rgb_tid);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_BGR,GL_UNSIGNED_BYTE,(void*)img.data);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depth_tid);
		glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE32F_ARB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (void*)dimg.data);
		

		test_program->Bind();

	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    glMatrixMode(GL_MODELVIEW);
	    glLoadIdentity();


	    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		test_program->Unbind();


		
		// glDrawArrays(GL_POINTS, 0, width*height);  // GPU-PASS


	    // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	    // glDisableClientState(GL_VERTEX_ARRAY);
	    // glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	    // glDisable(GL_TEXTURE_2D);

		/*pangolin texture testing

       	imageTexture.Upload(img.data,GL_RGB,GL_UNSIGNED_BYTE);
    	imageTexture.RenderToViewport();*/
		// glBindTexture(GL_TEXTURE_2D, 0);
	    pangolin::FinishFrame();
		/*debug code		
		double minVal; 
		double maxVal; 
		Point minLoc; 
		Point maxLoc;
		minMaxLoc( dimg, &minVal, &maxVal, &minLoc, &maxLoc );
		std::cout << "min val: " << minVal << std::endl;
		std::cout << "max val: " << maxVal << std::endl;
		*/

	}
	return 0;
}