#ifndef _SPRITE_HH_
#define _SPRITE_HH_

// A representation of Sprite that shall be drawn
class Sprite
{
public:
//private:
    int			x1;
    int			x2;

    // for line side calculation
    double		ggx;
    double		ggy;		

    // global bottom / top for silhouette clipping
    double		ggz;
    double		ggzt;

    // horizontal position of x1
    double		sstartfrac;
    
    double		sscale;
    
    // negative if flipped
    double		xxiscale;	

    double		ttexturemid;
    int			patch;

    // for color translation and shadow draw,
    //  maxbright frames as well
    lighttable_t*	colormap;
   
    int			mobjflags;
};

#endif
