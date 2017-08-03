
#include "m_pd.h"
#include "math.h"

#define PI M_PI

typedef struct _bcoeff{
    t_object  x_obj;
    t_float   x_freq;
    t_float   x_q_s;
    t_float   x_gain;
    t_float   x_type;
} t_bcoeff;

static t_class *bcoeff_class;

static void bcoeff_bang(t_bcoeff *x){
    t_atom at[5];
    double a0, a1, a2, b1, b2;
// off
    if (x->x_type == 0){
        a0 = 1.;
        a1 = a2 = b1 = b2 = 0.;
        }
// allpass
    else if (x->x_type == 1){
        double omega, alphaQ, cos_w, b0;
        double hz = (double)x->x_freq;
        double q = (double)x->x_q_s;
        double amp = pow(10, x->x_gain / 40);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        omega = hz * PI/nyq;
        
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ + 1;
        a0 = (1 - alphaQ) / b0;
        a1 = -2*cos_w / b0;
        a2 = 1;
        b1 = -a1;
        b2 = (alphaQ - 1) / b0;
    }
// bandpass
    else if (x->x_type == 2){
        double omega, alphaQ, cos_w, b0;
        double hz = (double)x->x_freq;
        double q = (double)x->x_q_s;
        double amp = pow(10, x->x_gain / 40);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        omega = hz * PI/nyq;
        
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ + 1;
        a0 = alphaQ / b0;
        a1 = 0.;
        a2 = -a0;
        b1 = 2*cos_w / b0;
        b2 = (alphaQ - 1) / b0;
    }
// bandstop
    else if (x->x_type == 3){
        double omega, alphaQ, cos_w, b0;
        double hz = (double)x->x_freq;
        double q = (double)x->x_q_s;
        double amp = pow(10, x->x_gain / 40);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        omega = hz * PI/nyq;
        
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ + 1;
        a0 = 1 / b0;
        a1 = -2*cos_w / b0;
        a2 = a0;
        b1 = -a1;
        b2 = (alphaQ - 1) / b0;
    }
// eq
    else if (x->x_type == 4){
        double omega, alphaQ, cos_w, b0;
        double hz = (double)x->x_freq;
        double q = (double)x->x_q_s;
        double amp = pow(10, x->x_gain / 40);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        omega = hz * PI/nyq;
        
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ/amp + 1;
        a0 = (1 + alphaQ*amp) / b0;
        a1 = -2*cos_w / b0;
        a2 = (1 - alphaQ*amp) / b0;
        b1 = 2*cos_w / b0;
        b2 = (alphaQ/amp - 1) / b0;
    }
// highpass
    else if (x->x_type == 5){
        double omega, alphaQ, cos_w, b0;
        double hz = (double)x->x_freq;
        double q = (double)x->x_q_s;
        double amp = pow(10, x->x_gain / 40);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        omega = hz * PI/nyq;
        
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ + 1;
        a0 = (1 + cos_w) / (2 * b0);
        a1 = -(1 + cos_w) / b0;
        a2 = a0;
        b1 = 2*cos_w / b0;
        b2 = (alphaQ - 1) / b0;
    }
// highshelf
    else if (x->x_type == 6){
        double omega, alphaS, cos_w, b0;
        double hz = (double)x->x_freq;
        double slope = (double)x->x_q_s;
        double amp = pow(10, x->x_gain / 40);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (slope < 0.000001)
            slope = 0.000001;
        if (slope > 1)
            slope = 1;
        omega = hz * PI/nyq;
        
        alphaS = sin(omega) * sqrt((amp*amp + 1) * (1/slope - 1) + 2*amp);
        cos_w = cos(omega);
        b0 = (amp+1) - (amp-1)*cos_w + alphaS;
        a0 = amp*(amp+1 + (amp-1)*cos_w + alphaS) / b0;
        a1 = -2*amp*(amp-1 + (amp+1)*cos_w) / b0;
        a2 = amp*(amp+1 + (amp-1)*cos_w - alphaS) / b0;
        b1 = -2*(amp-1 - (amp+1)*cos_w) / b0;
        b2 = -(amp+1 - (amp-1)*cos_w - alphaS) / b0;
    }
// lowpass
    else if (x->x_type == 7){
        double omega, alphaQ, cos_w, b0;
        double hz = (double)x->x_freq;
        double q = (double)x->x_q_s;
        double amp = pow(10, x->x_gain / 40);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        omega = hz * PI/nyq;
        
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ + 1;
        a0 = (1 - cos_w) / (2 * b0);
        a1 = (1 - cos_w) / b0;
        a2 = a0;
        b1 = 2*cos_w / b0;
        b2 = (alphaQ - 1) / b0;
    }
// lowshelf
    else if (x->x_type == 8){
        double omega, alphaS, cos_w, b0;
        double hz = (double)x->x_freq;
        double slope = (double)x->x_q_s;
        double amp = pow(10, x->x_gain / 40);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (slope < 0.000001)
            slope = 0.000001;
        if (slope > 1)
            slope = 1;
        omega = hz * PI/nyq;
        
        alphaS = sin(omega) * sqrt((amp*amp + 1) * (1/slope - 1) + 2*amp);
        cos_w = cos(omega);
        b0 = (amp+1) + (amp-1)*cos_w + alphaS;
        a0 = amp*(amp+1 - (amp-1)*cos_w + alphaS) / b0;
        a1 = 2*amp*(amp-1 - (amp+1)*cos_w) / b0;
        a2 = amp*(amp+1 - (amp-1)*cos_w - alphaS) / b0;
        b1 = 2*(amp-1 + (amp+1)*cos_w) / b0;
        b2 = -(amp+1 + (amp-1)*cos_w - alphaS) / b0;
    }
// resonant
    else if (x->x_type == 9){
        double omega, alphaQ, cos_w, b0;
        double hz = (double)x->x_freq;
        double q = (double)x->x_q_s;
        double amp = pow(10, x->x_gain / 40);
        double nyq = sys_getsr() * 0.5;
        if (hz < 0.1)
            hz = 0.1;
        if (hz > nyq - 0.1)
            hz = nyq - 0.1;
        if (q < 0.000001)
            q = 0.000001; // prevent blow-up
        omega = hz * PI/nyq;
    
        alphaQ = sin(omega) / (2*q);
        cos_w = cos(omega);
        b0 = alphaQ + 1;
        a0 = alphaQ*q / b0;
        a1 = 0.;
        a2 = -a0;
        b1 = 2*cos_w / b0;
        b2 = (alphaQ - 1) / b0;
    }
    SETFLOAT(at, b1);
    SETFLOAT(at+1, b2);
    SETFLOAT(at+2, a0);
    SETFLOAT(at+3, a1);
    SETFLOAT(at+4, a2);
    outlet_list(x->x_obj.ob_outlet, &s_list, 5, at);
}

void bcoeff_list(t_bcoeff *x, t_symbol *s, int ac, t_atom *av){
    // freq, gain, q
}

void bcoeff_off(t_bcoeff *x, t_symbol *s, int ac, t_atom *av){
    x->x_type = 0;
    bcoeff_bang(x);
}

void bcoeff_apass(t_bcoeff *x, t_symbol *s, int ac, t_atom *av){
    x->x_type = 1;
    bcoeff_bang(x);
}

void bcoeff_bandpass(t_bcoeff *x, t_symbol *s, int ac, t_atom *av){
    x->x_type = 2;
    bcoeff_bang(x);
}

void bcoeff_bandstop(t_bcoeff *x, t_symbol *s, int ac, t_atom *av){
    x->x_type = 3;
    bcoeff_bang(x);
}

void bcoeff_eq(t_bcoeff *x, t_symbol *s, int ac, t_atom *av){
    x->x_type = 4;
    bcoeff_bang(x);
}

void bcoeff_highpass(t_bcoeff *x, t_symbol *s, int ac, t_atom *av){
    x->x_type = 5;
    bcoeff_bang(x);
}

void bcoeff_highshelf(t_bcoeff *x, t_symbol *s, int ac, t_atom *av){
    x->x_type = 6;
    bcoeff_bang(x);
}

void bcoeff_lowpass(t_bcoeff *x, t_symbol *s, int ac, t_atom *av){
    x->x_type = 7;
    bcoeff_bang(x);
}

void bcoeff_lowshelf(t_bcoeff *x, t_symbol *s, int ac, t_atom *av){
    x->x_type = 8;
    bcoeff_bang(x);
}

void bcoeff_resonant(t_bcoeff *x, t_symbol *s, int ac, t_atom *av){
    x->x_type = 9;
    bcoeff_bang(x);
}

/////////////////////////////////

static void bcoeff_freq(t_bcoeff *x, t_floatarg val){
    x->x_freq = val;
    bcoeff_bang(x);
}

static void bcoeff_Q_S(t_bcoeff *x, t_floatarg val){
    x->x_q_s = val;
    bcoeff_bang(x);
}

static void bcoeff_gain(t_bcoeff *x, t_floatarg val){
    x->x_gain = val;
    bcoeff_bang(x);
}

static void *bcoeff_new(t_symbol *s, int argc, t_atom *argv){
    t_bcoeff *x = (t_bcoeff *)pd_new(bcoeff_class);
    float freq = 0;
    float q_or_s = 1;
    float db = 0;
    int type = 0;
/////////////////////////////////////////////////////////////////////////////////////
    int argnum = 0;
    while(argc > 0){
        if (argv -> a_type == A_SYMBOL){
            t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
            if(strcmp(curarg->s_name, "apass")==0){
                type = 1;
                argc--;
                argv++;
            }
            else if(strcmp(curarg->s_name, "bandpass")==0){
                type = 2;
                argc--;
                argv++;
            }
            else if(strcmp(curarg->s_name, "bandstop")==0){
                type = 3;
                argc--;
                argv++;
            }
            else if(strcmp(curarg->s_name, "eq")==0){
                type = 4;
                argc--;
                argv++;
            }
            else if(strcmp(curarg->s_name, "highpass")==0){
                type = 5;
                argc--;
                argv++;
            }
            else if(strcmp(curarg->s_name, "highshelf")==0){
                type = 6;
                argc--;
                argv++;
            }
            else if(strcmp(curarg->s_name, "lowpass")==0){
                type = 7;
                argc--;
                argv++;
            }
            else if(strcmp(curarg->s_name, "lowshelf")==0){
                type = 8;
                argc--;
                argv++;
            }
            else if(strcmp(curarg->s_name, "resonant")==0){
                type = 9;
                argc--;
                argv++;
            }
            else
                goto errstate;
        }
        if(argv -> a_type == A_FLOAT) {
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    freq = argval;
                    break;
                case 1:
                    q_or_s = argval;
                    break;
                case 2:
                    db = argval;
                    break;
                default:
                    break;
                };
            argnum++;
            argc--;
            argv++;
            }
    };
/////////////////////////////////////////////////////////////////////////////////////
    x->x_freq = freq;
    x->x_q_s = q_or_s;
    x->x_gain = db;
    x->x_type = type;
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft2"));
    outlet_new((t_object *)x, &s_list);
    return (x);
    errstate:
        pd_error(x, "bcoeff: improper args");
        return NULL;
}

void bcoeff_setup(void){
    bcoeff_class = class_new(gensym("bcoeff"), (t_newmethod)bcoeff_new, 0,
			    sizeof(t_bcoeff), 0, A_GIMME, 0);
    class_addbang(bcoeff_class, bcoeff_bang);
    class_addfloat(bcoeff_class, bcoeff_freq);
    class_addmethod(bcoeff_class, (t_method)bcoeff_Q_S, gensym("ft1"), A_FLOAT, 0);
    class_addmethod(bcoeff_class, (t_method)bcoeff_gain, gensym("ft2"), A_FLOAT, 0);
    
    class_addmethod(bcoeff_class, (t_method) bcoeff_lowpass, gensym("lowpass"), A_GIMME, 0);
    class_addmethod(bcoeff_class, (t_method) bcoeff_highpass, gensym("highpass"), A_GIMME, 0);
    class_addmethod(bcoeff_class, (t_method) bcoeff_bandpass, gensym("bandpass"), A_GIMME, 0);
    class_addmethod(bcoeff_class, (t_method) bcoeff_bandstop, gensym("bandstop"), A_GIMME, 0);
    class_addmethod(bcoeff_class, (t_method) bcoeff_resonant, gensym("resonant"), A_GIMME, 0);
    class_addmethod(bcoeff_class, (t_method) bcoeff_eq, gensym("eq"), A_GIMME, 0);
    class_addmethod(bcoeff_class, (t_method) bcoeff_lowshelf, gensym("lowshelf"), A_GIMME, 0);
    class_addmethod(bcoeff_class, (t_method) bcoeff_highshelf, gensym("highshelf"), A_GIMME, 0);
    class_addmethod(bcoeff_class, (t_method) bcoeff_apass, gensym("apass"), A_GIMME, 0);
}
