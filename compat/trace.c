/* Crash tracing for the EPOC port: -finstrument-functions hooks keep a
 * shadow call stack + ring of recent calls; the exception handler in
 * epoccons.cpp dumps them to trace.txt. Not instrumented itself. */
#define NOI __attribute__((no_instrument_function))
#define SHADOW_MAX 256
#define RING_MAX 64
extern int main(int, char **);
unsigned int epoc_tr_shadow[SHADOW_MAX];
int epoc_tr_depth = 0;
unsigned int epoc_tr_ring[RING_MAX];
unsigned int epoc_tr_ringpos = 0;
unsigned int epoc_tr_minsp = 0xFFFFFFFFu;
unsigned int epoc_tr_mainaddr = 0;

void NOI __cyg_profile_func_enter(void *fn, void *site)
{
	unsigned int sp = (unsigned int)&sp;
	(void)site;
	if (sp < epoc_tr_minsp) epoc_tr_minsp = sp;
	if (epoc_tr_depth < SHADOW_MAX) epoc_tr_shadow[epoc_tr_depth] = (unsigned int)fn;
	epoc_tr_depth++;
	epoc_tr_ring[epoc_tr_ringpos++ % RING_MAX] = (unsigned int)fn;
}
void NOI __cyg_profile_func_exit(void *fn, void *site)
{
	(void)fn; (void)site;
	if (epoc_tr_depth > 0) epoc_tr_depth--;
}
void NOI epoc_trace_init(void)
{
	unsigned int sp = (unsigned int)&sp;
	epoc_tr_mainaddr = (unsigned int)main;
	epoc_tr_minsp = sp;
}
