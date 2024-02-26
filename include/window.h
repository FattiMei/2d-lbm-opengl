#ifndef __WINDOW_H__
#define __WINDOW_H__


int  window_init(const char *title, int width, int height);
void window_set_hints(const int hints[][2], int n);
void window_set_callbacks();
int  window_should_close();
void window_swap_buffers();
void window_poll_events();
void window_close();


#endif
