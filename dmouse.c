#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <X11/Xlib.h>


static struct {
	Display *dpy;

	struct {
		int w,h;
	} screen;

	struct {
		int x,y;
	} mouse;

	int zoom;
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

	printf("coord %u %u, zoom %u\n",data.mouse.x,data.mouse.y,data.zoom);

	halve(argv[1][0]);

	printf("warping to %u %u, zoom %u\n",data.mouse.x,data.mouse.y,data.zoom);

	warp();
	save_coord();

	close_x();
}

#define MAX(x,y) ((x>y)?(x):(y))

void halve(char q) {
	int x,y;

	switch(q) {
	case 'q': x=-1; y=-1; break;
	case 'w': x= 0; y=-1; break;
	case 'e': x= 1; y=-1; break;

	case 'a': x=-1; y= 0; break;
	case 's': x= 0; y= 0; break;
	case 'd': x= 1; y= 0; break;

	case 'z': x=-1; y= 1; break;
	case 'x': x= 0; y= 1; break;
	case 'c': x= 1; y= 1; break;

	case 'R': reset(); return;

	case 'l': button(0); return;
	case 'm': button(1); return;
	case 'r': button(2); return;
	}

	int fx=MAX(1,data.screen.w/data.zoom);
	int fy=MAX(1,data.screen.h/data.zoom);

	printf("fx %u fy %u\n",fx,fy);

	data.mouse.x=data.mouse.x+x*fx;
	data.mouse.y=data.mouse.y+y*fy;

	if(fx>1 || fy>1) data.zoom*=3;
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
		data.zoom=3;
		return;
	}

	read(f,&data.mouse.x,sizeof(data.mouse.x));
	read(f,&data.mouse.y,sizeof(data.mouse.y));
	read(f,&data.zoom,sizeof(data.zoom));

	close(f);
}

void save_coord(void) {
	chdir(getenv("HOME"));
	int f=open(".dmouse.new",O_CREAT|O_TRUNC|O_WRONLY,0600);

	write(f,&data.mouse.x,sizeof(data.mouse.x));
	write(f,&data.mouse.y,sizeof(data.mouse.y));
	write(f,&data.zoom,sizeof(data.zoom));

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

void button(int b) {
	XEvent event;
	
	memset(&event,0,sizeof(event));
	
	event.type=ButtonPress;
	event.xbutton.button=b;
	event.xbutton.same_screen=True;
	
	XQueryPointer(data.dpy,RootWindow(data.dpy,DefaultScreen(data.dpy)),&event.xbutton.root,&event.xbutton.window,&event.xbutton.x_root,&event.xbutton.y_root,&event.xbutton.x,&event.xbutton.y,&event.xbutton.state);
	event.xbutton.subwindow=event.xbutton.window;
	
	while(event.xbutton.subwindow)
	{
		event.xbutton.window=event.xbutton.subwindow;
		XQueryPointer(data.dpy,event.xbutton.window,&event.xbutton.root,&event.xbutton.subwindow,&event.xbutton.x_root,&event.xbutton.y_root,&event.xbutton.x,&event.xbutton.y,&event.xbutton.state);
	}
	
	XSendEvent(data.dpy,PointerWindow,True,0xfff,&event);
	XFlush(data.dpy);
	
	usleep(100000);
	
	event.type=ButtonRelease;
	event.xbutton.state=0x100;
	
	XSendEvent(data.dpy,PointerWindow,True,0xfff,&event);
	XFlush(data.dpy);
}

