#include <gtk/gtk.h>
#include "PJ_RPI.h"

#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include "tlpi_hdr.h"

// compile with gcc gtktest.c -o gtktest -l PJ_RPI `pkg-config --cflags --libs gtk+-2.0`

struct portStruct
{
    int port;
    int freq;
} data;

struct itimerval itv;
struct sigaction sa;

int GPIO[] = {4, 17, 27, 22, 5, 6, 13, 19, 26, 18, 23, 24, 25, 12, 16, 20, 21};
//gpios die we kunnen gebruiken.
char state[27] = {0};

GtkWidget *lbl3;

static void sigalrmHandler(int sig)
{
    char port = data.port;
    if (!state[port])
    {
        GPIO_SET = (1 << port);
        printf("GPIO %i ON \r\n",port);
        state[port] = 1;
    }
    else
    {
        GPIO_CLR = (1 << port);
        printf("GPIO %i OFF \r\n",port);
         state[port] = 0;
    }
    
    char buffer[30];
    sprintf(buffer, "state GPIO %i: %i ", port, state[port]);
    gtk_label_set_text(GTK_LABEL(lbl3), buffer);
}

void set_freq(GtkWidget *wid, gpointer *ptr)
{
    data.freq = gtk_scale_get_value_pos(GTK_SCALE(wid));
}

void set_port(GtkWidget *wid, gpointer *ptr)
{
    data.port = atoi(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wid)));
}

char controlStateChange(int port)
{
    long input = GPIO_READ(port);

    if (input)
        state[port] = 1;
    else
        state[port] = 0;
    return state[port];
}

void showInput(GtkWidget *wid, gpointer *data)
{
    char *selected = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wid));
    char input = controlStateChange(atoi(selected));
    char buffer[30];
    sprintf(buffer, "state GPIO %s: %i ", selected, input);
    printf("state GPIO %s: %i \n", selected, input);
    gtk_label_set_text(GTK_LABEL(lbl3), buffer);
}

void setOutput(GtkWidget *wid, gpointer *data)
{
    char *selected = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wid));
    char port = atoi(selected);
    INP_GPIO(port);
    OUT_GPIO(port);
    if (!state[port])
    {
        GPIO_SET = (1 << port);
        state[port] = 1;
    }
    else
    {
        GPIO_CLR = (1 << port);
        state[port] = 0;
    }

    char buffer[30];
    sprintf(buffer, "state GPIO %s: %i ", selected, state[port]);
    printf("state GPIO %s: %i \n", selected, state[port]);
    gtk_label_set_text(GTK_LABEL(lbl3), buffer);
}

void setTimer(GtkWidget *wid, gpointer ptr)
{
    itv.it_interval.tv_sec = data.freq;
    if (setitimer(ITIMER_REAL, &itv, NULL) == -1)  
      printf("setitimer");

}

void end_program(GtkWidget *wid, gpointer ptr)
{
    gtk_main_quit();
}
int main(int argc, char *argv[])
{
    if (map_peripheral(&gpio) == -1)
    {
        printf("Failed to map the physical GPIO registers into the virtual memory space.\n");
        return -1;
    }

    gtk_init(&argc, &argv);
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *btn = gtk_button_new_with_label("Set timer");
    GtkWidget *comb1 = gtk_combo_box_text_new();
    GtkWidget *comb2 = gtk_combo_box_text_new();
    GtkWidget *comb3 = gtk_combo_box_text_new();
    GtkWidget *comb4 = gtk_combo_box_text_new();
    GtkWidget *lbl1 = gtk_label_new("Toggle Gpio ");
    GtkWidget *lbl2 = gtk_label_new("Show Gpio ");
    GtkWidget *tbl = gtk_table_new(9, 4, TRUE);
    GtkWidget *scale = gtk_hscale_new_with_range(0, 10, 1);

    lbl3 = gtk_label_new("Messages");
    gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);

    struct portStruct data = {17, 10};

    g_signal_connect(win, "delete_event", G_CALLBACK(end_program), NULL);
    g_signal_connect(comb1, "changed", G_CALLBACK(setOutput), NULL);
    g_signal_connect(comb2, "changed", G_CALLBACK(showInput), NULL);
    g_signal_connect(comb3, "changed", G_CALLBACK(set_port), NULL);
    g_signal_connect(scale, "value-changed", G_CALLBACK(set_freq), NULL);
    g_signal_connect(btn, "clicked", G_CALLBACK(setTimer), NULL);

    for (int i = 0; i < sizeof(GPIO) / sizeof(GPIO[0]); i++)
    {
        char buf[4];
        sprintf(buf, "%i", GPIO[i]);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comb1), buf);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comb2), buf);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comb3), buf);
    }

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sigalrmHandler;
    if (sigaction(SIGALRM, &sa, NULL) == -1)
        printf("sigaction");

    itv.it_value.tv_sec = 3;
    itv.it_value.tv_usec = 0;
    itv.it_interval.tv_sec = 3;
    itv.it_interval.tv_usec = 0;

    gtk_table_set_row_spacings(GTK_TABLE(tbl), 7);

    gtk_table_attach_defaults(GTK_TABLE(tbl), lbl1, 1, 2, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(tbl), comb1, 2, 3, 1, 2);

    gtk_table_attach_defaults(GTK_TABLE(tbl), lbl2, 1, 2, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(tbl), comb2, 2, 3, 2, 3);

    gtk_table_attach_defaults(GTK_TABLE(tbl), comb3, 1, 2, 4, 5);
    gtk_table_attach_defaults(GTK_TABLE(tbl), scale, 2, 3, 4, 5);
    gtk_table_attach_defaults(GTK_TABLE(tbl), btn, 1, 3, 5, 6);

    gtk_table_attach_defaults(GTK_TABLE(tbl), lbl3, 1, 3, 7, 8);

    gtk_container_add(GTK_CONTAINER(win), tbl);

    gtk_widget_show_all(win);
    gtk_main();
}