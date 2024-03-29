#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>


static struct {
	Display *dpy;

	struct {
		int w,h;
	} screen;

	struct {
		int x,y;
	} mouse;

	struct {
		int x,y;
	} zoom;
} data;

void open_x(void);
void close_x(void);

void reset(void);
void button(int);
void read_info(void);
void read_coord(void);
void halve(char s);
void warp(void);
void save_coord(void);

int main(int argc,char *argv[]) {
	chdir(getenv("HOME"));

	open_x();

	read_info();

	printf("screen %u %u\n",data.screen.w,data.screen.h);

	read_coord();

	printf("coord %u %u, zoom %u %u\n",data.mouse.x,data.mouse.y,data.zoom.x,data.zoom.y);

	halve(argv[1][0]);

	printf("warping to %u %u, zoom %u %u\n",data.mouse.x,data.mouse.y,data.zoom.x,data.zoom.y);

	warp();
	save_coord();

	close_x();
}

#define MAX(x,y) ((x>y)?(x):(y))

void halve(char q) {
	int x=0,y=0;

	switch(q) {
	case 'w': x= 0; y=-1; break;
	case 's': x= 0; y= 1; break;

	case 'a': x=-1; y= 0; break;
	case 'd': x= 1; y= 0; break;

	case 'e': reset(); return;

	case 'q': button(Button1); return;
	case 'c': button(Button2); return;
	case 'x': button(Button3); return;

	default: reset(); return;
	}

	int fx=MAX(1,data.screen.w/data.zoom.x);
	int fy=MAX(1,data.screen.h/data.zoom.y);

	printf("fx %u fy %u\n",fx,fy);

	data.mouse.x=data.mouse.x+x*fx;
	data.mouse.y=data.mouse.y+y*fy;

	if(x && fx>1) data.zoom.x*=2;
	if(y && fy>1) data.zoom.y*=2;
}

void reset(void) {
	unlink(".dmouse");
	read_coord();
}


void read_info(void) {
	data.screen.w=XWidthOfScreen(DefaultScreenOfDisplay(data.dpy));
	data.screen.h=XHeightOfScreen(DefaultScreenOfDisplay(data.dpy));
}

void read_coord(void) {
	int f=open(".dmouse",O_RDONLY);
	if(f==-1) {
		data.mouse.x=data.screen.w/2;
		data.mouse.y=data.screen.h/2;
		data.zoom.x=4;
		data.zoom.y=4;
		return;
	}

	read(f,&data.mouse.x,sizeof(data.mouse.x));
	read(f,&data.mouse.y,sizeof(data.mouse.y));
	read(f,&data.zoom.x,sizeof(data.zoom.x));
	read(f,&data.zoom.y,sizeof(data.zoom.y));

	close(f);
}

void save_coord(void) {
	chdir(getenv("HOME"));
	int f=open(".dmouse.new",O_CREAT|O_TRUNC|O_WRONLY,0600);

	write(f,&data.mouse.x,sizeof(data.mouse.x));
	write(f,&data.mouse.y,sizeof(data.mouse.y));
	write(f,&data.zoom.x,sizeof(data.zoom.x));
	write(f,&data.zoom.y,sizeof(data.zoom.y));

	close(f);

	rename(".dmouse.new",".dmouse");
}


void warp() {
	XWarpPointer(data.dpy,None,RootWindow(data.dpy,DefaultScreen(data.dpy)),0,0,0,0,data.mouse.x,data.mouse.y);
	XSetInputFocus(data.dpy,PointerRoot,RevertToPointerRoot,CurrentTime);
}

void open_x(void) {
	data.dpy=XOpenDisplay(0);
}

void close_x(void) {
	XCloseDisplay(data.dpy);
}

static void _button(int b,int press) {
	XButtonEvent event;
	
	memset(&event,0,sizeof(event));
	
	event.type=press?ButtonPress:ButtonRelease;
	event.button=b;
	event.root=RootWindow(data.dpy,DefaultScreen(data.dpy));
	event.same_screen=True;
	event.display=data.dpy;
	event.subwindow=None;
	event.time=CurrentTime;
	
	XQueryPointer(data.dpy,RootWindow(data.dpy,DefaultScreen(data.dpy)),&event.root,&event.window,&event.x_root,&event.y_root,&event.x,&event.y,&event.state);
	event.subwindow=event.window;
	
	while(event.subwindow)
	{
		event.window=event.subwindow;
		XQueryPointer(data.dpy,event.window,&event.root,&event.subwindow,&event.x_root,&event.y_root,&event.x,&event.y,&event.state);
	}

	if(!press) {
		switch(b) {
		case Button1: event.state |= Button1MotionMask; break;
		case Button2: event.state |= Button2MotionMask; break;
		case Button3: event.state |= Button3MotionMask; break;
		}
	}
	
	XSendEvent(data.dpy,PointerWindow,True,ButtonPressMask,(XEvent *)&event);
	XFlush(data.dpy);
	
	reset();
}

void button(int b) {
	_button(b,1);
	usleep(100000);
	_button(b,0);
}

