// porres 2020

#include <math.h>
#include "m_pd.h"
#include "magic.h" // needed for toggle start
#include "buffer.h"
#include <stdlib.h>

#define HALF_PI (3.14159265358979323846 * 0.5)

#define ONE_SIXTH 0.16666666666666666666667f
#define SHARED_FLT_MAX  1E+36

typedef struct _tabplayer{
    t_object    x_obj;
    t_buffer   *x_buffer;
    t_glist    *x_glist;
    int         x_hasfeeders; //if there's a signal coming in the main inlet
    float       x_sr_khz; //pd's sample rate
    float       x_array_sr_khz; //array's sample rate in ms
    double      x_sr_ratio; //sample rate ratio (array/pd)
    float       x_stms; //start position in ms
    float       x_endms; //end pos in ms
    unsigned long long x_npts; //array size in samples
    unsigned long long x_start; //start position in samp
    unsigned long long x_end; //end pos in samp
    unsigned long long x_rangesamp; //length of loop in samples
    unsigned long long x_fadesamp; //length of fade in samples
    float       x_fadems; // fade time (fade-in starts at stms, fade-out at endms in xfade mode)
    double      x_rate; // rate of playback
    double      x_phase;
    int         x_xfade; // flag to set to xfade mode
    int         x_pos; // phase/play position
    int         x_loop; //if loop or not
    int         x_playing; //if playing
    int         x_playnew; //if started playing this particular block
    int         x_n_ch;
    t_float    *x_ivec; // input vector
    t_float   **x_ovecs; //out vectors
    t_outlet   *x_donelet;
}t_play;

static t_class *tabplayer_class;

static void tabplayer_fade_check(t_play *x, t_floatarg f){
    x->x_fadesamp  = (unsigned long long)(f * x->x_array_sr_khz);
    if(x->x_fadesamp > (x->x_rangesamp / 2))
        x->x_fadesamp = (x->x_rangesamp / 2);
}

static void tabplayer_fade(t_play *x, t_floatarg f){
    x->x_fadems = f < 0 ? 0 : f;
    tabplayer_fade_check(x, x->x_fadems);
}

static void tabplayer_xfade(t_play *x, t_floatarg f){
    x->x_xfade = f != 0;
}

static void tabplayer_range_check(t_play *x){
    if(x->x_start > x->x_end){
        unsigned long long temp = x->x_start;
        x->x_start = x->x_end;
        x->x_end = temp;
    }
    x->x_rangesamp = x->x_end - x->x_start;
    tabplayer_fade_check(x, x->x_fadems);
}

static void tabplayer_range(t_play *x, t_floatarg f1, t_floatarg f2){
    x->x_start = f1 < 0 ? 0 : f1 > x->x_npts ? x->x_npts : (unsigned long long)(f1 * x->x_npts);
    x->x_end = f2 < 0 ? 0 : f2 > x->x_npts ? x->x_npts : (unsigned long long)(f2 * x->x_npts);
    tabplayer_range_check(x);
}

static unsigned long long tabplayer_ms2samp(t_play *x, t_floatarg f){
    unsigned long long samp = (unsigned long long)(f * x->x_array_sr_khz);
    if(samp > x->x_npts)
        samp = x->x_npts;
    else if(samp < 0)
        samp = 0;
    return(samp);
}

static void tabplayer_start(t_play *x, t_floatarg f){
    x->x_start = tabplayer_ms2samp(x, f);
    tabplayer_range_check(x);
}

static void tabplayer_end(t_play *x, t_floatarg f){
    x->x_end = tabplayer_ms2samp(x, f);
    tabplayer_range_check(x);
}

static void tabplayer_reset(t_play *x){
    x->x_start = 0;
    x->x_end = x->x_rangesamp = x->x_npts;
    tabplayer_fade_check(x, x->x_fadems);
}

static void tabplayer_speed(t_play *x, t_floatarg f){
    x->x_rate = f / 100;
}

/*static void tabplayer_arraysr(t_play *x, t_floatarg f){
    //sample rate of array in samp/sec
    if(f <= 1)
        f = 1;
    x->x_array_sr_khz = f * 0.001;
    x->x_sr_ratio = x->x_array_sr_khz/x->x_sr_khz;
    tabplayer_ms2samp(x);
}*/

static void tabplayer_set(t_play *x, t_symbol *s){
    buffer_setarray(x->x_buffer, s);
    x->x_npts = x->x_buffer->c_npts;
    x->x_start = 0;
    x->x_end = x->x_rangesamp = x->x_npts;
    x->x_sr_ratio = x->x_array_sr_khz/x->x_sr_khz;
}

static void tabplayer_pos(t_play *x, t_floatarg f){
    x->x_pos = 1;
    double pos = f < 0 ? 0 : f > 1 ? 1 : (double)f;
    x->x_phase = pos * x->x_rangesamp + x->x_start;
    x->x_playing = x->x_playnew = 1; // start playing
}

static void tabplayer_bang(t_play *x){
    x->x_pos = 0;
    x->x_playing = x->x_playnew = 1; // start playing
}

static void tabplayer_play(t_play *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac){ // args: start (ms) / end (ms), rate
        float stms = 0;
        float endms = SHARED_FLT_MAX;
        float rate = 1;
        int argnum = 0;
        while(ac){
            if(av->a_type == A_FLOAT){
                switch(argnum){
                    case 0:
                        stms = atom_getfloatarg(0, ac, av);
                        break;
                    case 1:
                        endms = atom_getfloatarg(0, ac, av);
                        break;
                    case 2:
                        rate = atom_getfloatarg(0, ac, av) * 0.01;
                        break;
                    default:
                        break;
                };
                argnum++;
            };
            ac--, av++;
        };
        x->x_start = tabplayer_ms2samp(x, stms);
        x->x_end = tabplayer_ms2samp(x, endms);
        tabplayer_range_check(x);
    }
    tabplayer_bang(x);
}

static void tabplayer_stop(t_play *x){
    if(x->x_playing){
        x->x_playing = x->x_playnew = 0;
        outlet_bang(x->x_donelet);
    };
}

static void tabplayer_float(t_play *x, t_floatarg f){
    f > 0 ? tabplayer_bang(x) : tabplayer_stop(x);
}

static void tabplayer_pause(t_play *x){
    x->x_playing = 0;
}

static void tabplayer_resume(t_play *x){
    x->x_playing = 1;
}

static void tabplayer_loop(t_play *x, t_floatarg f){
    x->x_loop = f > 0 ? 1 : 0;
}

static double tabplayer_fadeInOut(t_play *x, double phase, double out){
    
    /* if(x->x_loop && x->x_xfade){  // check loop conditions
        x->x_stxsamp = x->x_start + x->x_fadesamp;
        if(phase < x->x_stxsamp){ // in xfade period, get fade gain }*/
    
    if(phase < (x->x_start + x->x_fadesamp)){
        double fade = (phase - x->x_start) / x->x_fadesamp;
        out *= sin(fade*HALF_PI);
    }
    else if(phase > (x->x_end - x->x_fadesamp)){
        double fade = (phase - (x->x_end - x->x_fadesamp)) / x->x_fadesamp;
        out *= cos(fade*HALF_PI);
    }
    return(out);
}

static double tabplayer_interp(t_play *x, int chidx, double phase){
    double out = 0.;
    t_word **vectable = x->x_buffer->c_vectors; // ??
    t_word *vp = vectable[chidx]; // ??
    if(vp){
        float f,  a,  b,  c,  d, cmb;
        int maxindex = x->x_npts - 3;
        if(phase < 0 || phase > maxindex)
            phase = 0;  // CHECKED: a value 0, not ndx 0 (???)
        int ndx = (int)phase;
        if(ndx < 1){
            ndx = 1;
            f = 0;
        }
        else if(ndx > maxindex){
            ndx = maxindex;
            f = 1;
        }
        else
            f = phase - ndx;
        vp += ndx;
        a = vp[-1].w_float;
        b = vp[0].w_float;
        c = vp[1].w_float;
        d = vp[2].w_float;
        cmb = c-b;
        out = b + f*(cmb - ONE_SIXTH*(1. - f)*((d - a - 3.0f*cmb)*f + (d + 2.0f*a - 3.0f*b)));
        if(x->x_fadesamp > 0)
            out = tabplayer_fadeInOut(x, phase, out);
    }
    return(out);
}

static t_int *tabplayer_perform(t_int *w){
    t_play *x = (t_play *)(w[1]);
    t_buffer *buffer = x->x_buffer;
    int n = (int)(w[2]);
    int chidx, i;
    if(buffer->c_playable){
        if(x->x_hasfeeders){ // signal input present, indexing into array
            t_float *xin = x->x_ivec;
            for(i = 0; i < n; i++){
                float phase = *xin++;
                for(chidx = 0; chidx < x->x_n_ch; chidx++){
                    t_float *out = *(x->x_ovecs+chidx);
                    out[i] = tabplayer_interp(x, chidx, phase);
                };
            };
        }
        else{ // no signal input present, auto playback mode
            if(!x->x_playing) // not playing, out zeros
                goto nullstate;
            else{ // PLAYING
                int isneg = x->x_rate < 0;
                if(x->x_playnew){
                    if(!x->x_pos)
                        x->x_phase = isneg ? (double)x->x_end : (double)x->x_start;
                    else
                        x->x_pos = 0;
                    x->x_playnew = 0;
                };
                for(i = 0; i < n; i++){
                    double phase = x->x_phase;
                    if(isneg){ // bounds checking backwards
                        if(phase < x->x_start){
                            if(x->x_loop){
                                double dif = (double)x->x_start - phase;
                                phase = (double)(x->x_end) - dif;
                                outlet_bang(x->x_donelet);
                            }
                            else{ // done playing
                                outlet_bang(x->x_donelet);
                                x->x_playing = 0;
                            };
                        }
                    }
                    else{ // bounds checking forwards
                        if(phase > x->x_end){
                            if(x->x_loop){
                                phase = (double)x->x_start + phase - (double)x->x_end;
                                outlet_bang(x->x_donelet);
                            }
                            else{ //we're done
                                outlet_bang(x->x_donelet);
                                x->x_playing = 0;
                            };
                        }
                    };
                    for(chidx = 0; chidx < x->x_n_ch; chidx++){ // get output
                        t_float *output = *(x->x_ovecs+chidx);
                        output[i] = tabplayer_interp(x, chidx, phase);
                    };
                    x->x_phase = phase + x->x_sr_ratio*x->x_rate; // increment for both forwards/backwards
                };
            }
        };
    }
    else{
        nullstate:
            for(chidx = 0; chidx < x->x_n_ch; chidx++){
                t_float *output = *(x->x_ovecs+chidx);
                int nblock = n;
                while(nblock--)
                    *output++ = 0;
            };
    };
    return(w+3);
}

static void tabplayer_dsp(t_play *x, t_signal **sp){
    buffer_checkdsp(x->x_buffer);
    unsigned long long npts = x->x_buffer->c_npts;
    x->x_hasfeeders = magic_inlet_connection((t_object *)x, x->x_glist, 0, &s_signal);
    t_float pdksr= sp[0]->s_sr * 0.001;
    x->x_sr_khz = pdksr;
    x->x_sr_ratio = (double)(x->x_array_sr_khz/x->x_sr_khz);
    if(npts != x->x_npts){
        x->x_npts = npts;
        tabplayer_reset(x); // recalculate sample equivalents
    };
    t_signal **sigp = sp;
    x->x_ivec = (*sigp++)->s_vec;
    for(int i = 0; i < x->x_n_ch; i++) //input vectors first
        *(x->x_ovecs+i) = (*sigp++)->s_vec;
    dsp_add(tabplayer_perform, 2, x, sp[0]->s_n);
}

static void *tabplayer_free(t_play *x){
    buffer_free(x->x_buffer);
    freebytes(x->x_ovecs, x->x_n_ch * sizeof(*x->x_ovecs));
    outlet_free(x->x_donelet);
    return(void *)x;
}

static void *tabplayer_new(t_symbol * s, int ac, t_atom *av){
    t_play *x = (t_play *)pd_new(tabplayer_class);
    t_symbol *arrname = NULL;
    t_float channels = 1;
    t_float fade = 0;
    x->x_xfade = 0;
    int loop = 0;
    int nameset = 0;
    while(ac){
        if(av->a_type == A_SYMBOL){ // if name not passed so far, count arg as array name
            s = atom_getsymbolarg(0, ac, av);
            if(s == gensym("-loop")){
                loop = 1;
                ac--, av++;
            }
            else if(s == gensym("-fade") && ac >= 2){
                fade = atom_getfloatarg(1, ac, av);
                ac-=2, av+=2;
            }
            else if(!nameset){
                arrname = s;
                ac--, av++;
                nameset = 1;
            }
            else
                goto errstate;
        }
        else{
            if(nameset){
                channels = atom_getfloatarg(0, ac, av);
                ac--, av++;
            }
            else
                goto errstate;
        }
    };
    // one auxiliary signal:  position input
    int chn_n = (int)channels > 64 ? 64 : (int)channels;
    x->x_glist = canvas_getcurrent();
    x->x_hasfeeders = 0;
    x->x_sr_khz = (float)sys_getsr() * 0.001;
    x->x_array_sr_khz = x->x_sr_khz; // set sample rate of array as pd's sample rate for now
    x->x_buffer = buffer_init((t_class *)x, arrname, chn_n, 0);
    if(x->x_buffer){
        int ch = x->x_buffer->c_numchans;
        x->x_npts = x->x_buffer->c_npts;
        x->x_n_ch = ch;
        x->x_ovecs = getbytes(x->x_n_ch * sizeof(*x->x_ovecs));
        while(ch--)
            outlet_new((t_object *)x, &s_signal);
        x->x_donelet = outlet_new(&x->x_obj, &s_bang);
        x->x_playing = 0;
        x->x_playnew = 0;
        x->x_rate = 1;
        tabplayer_reset(x);
        x->x_loop = loop;
        tabplayer_fade(x, fade);
    }
    return(x);
    errstate:
        pd_error(x, "[tabplayer~]: improper args");
        return(NULL);
}

void tabplayer_tilde_setup(void){
    tabplayer_class = class_new(gensym("tabplayer~"), (t_newmethod)tabplayer_new, (t_method)tabplayer_free,
        sizeof(t_play), 0, A_GIMME, 0);
    class_domainsignalin(tabplayer_class, -1);
    class_addbang(tabplayer_class, tabplayer_bang);
    class_addfloat(tabplayer_class, tabplayer_float);
    class_addmethod(tabplayer_class, (t_method)tabplayer_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(tabplayer_class, (t_method)tabplayer_set, gensym("set"), A_SYMBOL, 0);
    class_addmethod(tabplayer_class, (t_method)tabplayer_pos, gensym("pos"), A_FLOAT, 0);
    class_addmethod(tabplayer_class, (t_method)tabplayer_play, gensym("play"), A_GIMME, 0);
    class_addmethod(tabplayer_class, (t_method)tabplayer_stop, gensym("stop"), 0);
    class_addmethod(tabplayer_class, (t_method)tabplayer_pause, gensym("pause"), 0);
    class_addmethod(tabplayer_class, (t_method)tabplayer_resume, gensym("resume"), 0);
    class_addmethod(tabplayer_class, (t_method)tabplayer_reset, gensym("reset"), 0);
    class_addmethod(tabplayer_class, (t_method)tabplayer_start, gensym("start"), A_FLOAT, 0);
    class_addmethod(tabplayer_class, (t_method)tabplayer_end, gensym("end"), A_FLOAT, 0);
    class_addmethod(tabplayer_class, (t_method)tabplayer_range, gensym("range"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(tabplayer_class, (t_method)tabplayer_speed, gensym("speed"), A_FLOAT, 0);
    class_addmethod(tabplayer_class, (t_method)tabplayer_loop, gensym("loop"), A_FLOAT, 0);
    class_addmethod(tabplayer_class, (t_method)tabplayer_fade, gensym("fade"), A_FLOAT, 0);
    class_addmethod(tabplayer_class, (t_method)tabplayer_xfade, gensym("xfade"), A_FLOAT, 0);
//    class_addmethod(tabplayer_class, (t_method)tabplayer_arraysr, gensym("arraysr"), A_FLOAT, 0);
}
