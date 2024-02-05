#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <unistd.h>

static char *default_label = "";
static char *label = NULL;
static char *device_name = NULL;

static void
sinkcbk(pa_context *ctx, const pa_sink_info *i, int eol, void *userdata)
{
	if(!i)
		return;

	float volume = (float)pa_cvolume_avg(&i->volume) / (float)PA_VOLUME_NORM;
	if(i->mute)
		printf("MUTE\n");
	else
		printf("%s%0.0f\n", label, volume * 100);
	fflush(stdout);
}

static void
eventcbk(pa_context *ctx, pa_subscription_event_type_t t, uint32_t idx, void *data)
{
	pa_operation *op = NULL;
	if((t & (PA_SUBSCRIPTION_EVENT_SERVER | PA_SUBSCRIPTION_EVENT_CHANGE)))
		op = pa_context_get_sink_info_by_name(ctx, device_name, sinkcbk, data);

	if(op)
		pa_operation_unref(op);
}

static void
servercbk(pa_context *ctx, const pa_server_info *i, void *user)
{
	device_name = strdup(i->default_sink_name);
}

int
main(int argc, char *argv[])
{
	int c;
	pa_mainloop *mloop;
	pa_mainloop_api *mloopapi;
	pa_context *ctx;
	pa_context_state_t state;
	pa_operation *op;

	label = default_label;
	while((c = getopt(argc, argv, "d:l:")) > 0) {
		switch(c) {
		case 'd':
			device_name = strdup(optarg);
			break; 
		case 'l':
			label = strdup(optarg);
			break;
		}
	}

	mloop = pa_mainloop_new();
	mloopapi = pa_mainloop_get_api(mloop);
	ctx = pa_context_new(mloopapi, "DWMstatus audio");
	pa_context_connect(ctx, NULL, 0, NULL);

	/* wait until ready */
	while((state = pa_context_get_state(ctx)) != PA_CONTEXT_READY) {
		switch(state) {
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			goto end;
		default: ;/* do nothing */
		}
		pa_mainloop_iterate(mloop, 1, NULL);
	}

	op = pa_context_get_server_info(ctx, servercbk, NULL);
	pa_operation_unref(op);

	pa_context_set_subscribe_callback(ctx, eventcbk, NULL);
	pa_context_subscribe(ctx, PA_SUBSCRIPTION_EVENT_SERVER | PA_SUBSCRIPTION_EVENT_CHANGE, NULL, NULL);
	eventcbk(ctx, PA_SUBSCRIPTION_EVENT_SERVER | PA_SUBSCRIPTION_EVENT_CHANGE, 0, NULL);
	pa_mainloop_run(mloop, NULL);

end:
	pa_context_disconnect(ctx);
	pa_context_unref(ctx);
	pa_mainloop_free(mloop);

	return 0;
}
