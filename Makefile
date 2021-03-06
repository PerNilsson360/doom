################################################################
#
# $Id:$
#
# $Log:$
#
CC=  g++  # gcc or g++

CFLAGS=-g -O4 -Wall -DNORMALUNIX -DLINUX -fpermissive -std=gnu++14
LDFLAGS=-L/usr/X11R6/lib
LIBS=-lX11  -lm #-lnsl

# subdirectory for objects
O=linux

# not too sophisticated dependency
OBJS=				\
		$(O)/doomdef.o		\
		$(O)/doomstat.o		\
		$(O)/dstrings.o		\
		$(O)/i_system.o		\
		$(O)/i_sound.o		\
		$(O)/i_video.o		\
		$(O)/i_net.o			\
		$(O)/f_finale.o		\
		$(O)/f_wipe.o 		\
		$(O)/d_main.o			\
		$(O)/d_net.o			\
		$(O)/d_items.o		\
		$(O)/g_game.o			\
		$(O)/m_menu.o			\
		$(O)/m_misc.o			\
		$(O)/m_argv.o  		\
		$(O)/m_swap.o			\
		$(O)/m_cheat.o		\
		$(O)/m_random.o		\
		$(O)/am_map.o			\
		$(O)/p_ceilng.o		\
		$(O)/p_doors.o		\
		$(O)/p_enemy.o		\
		$(O)/p_floor.o		\
		$(O)/p_inter.o		\
		$(O)/p_lights.o		\
		$(O)/p_map.o			\
		$(O)/p_maputl.o		\
		$(O)/p_plats.o		\
		$(O)/p_pspr.o			\
		$(O)/p_setup.o		\
		$(O)/p_sight.o		\
		$(O)/p_spec.o			\
		$(O)/p_switch.o		\
		$(O)/p_telept.o		\
		$(O)/p_tick.o			\
		$(O)/p_saveg.o		\
		$(O)/p_user.o			\
		$(O)/r_bsp.o			\
		$(O)/r_data.o			\
		$(O)/r_draw.o			\
		$(O)/r_main.o			\
		$(O)/r_plane.o		\
		$(O)/r_segs.o			\
		$(O)/r_sky.o			\
		$(O)/r_things.o		\
		$(O)/w_wad.o			\
		$(O)/wi_stuff.o		\
		$(O)/v_video.o		\
		$(O)/st_lib.o			\
		$(O)/st_stuff.o		\
		$(O)/hu_stuff.o		\
		$(O)/hu_lib.o			\
		$(O)/s_sound.o\
		$(O)/z_zone.o\
		$(O)/info.o\
		$(O)/sounds.o	\
		$(O)/BlockMap.o \
		$(O)/BspNode.o \
		$(O)/DivLine.o \
		$(O)/Mob.o \
		$(O)/Angle.o \
		$(O)/ImageScaler.o \
		$(O)/MapThing.o \
		$(O)/DataInput.o \
		$(O)/Vertex.o \
		$(O)/Line.o \
		$(O)/Sector.o \
		$(O)/SubSector.o \
		$(O)/Side.o \
		$(O)/Seg.o \
		$(O)/BoundingBox.o \
		$(O)/SpriteFrame.o \
		$(O)/AnimatedSprite.o \

all:	$(O)/linuxxdoom
	find . -name "*.[chCH]" -print | etags -

clean:
	rm -f *.o *~ *.flc
	rm -f linux/*
	rm -f TAGS

$(O)/linuxxdoom:	$(CCOBJS) $(OBJS) $(O)/i_main.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(CCOBJS) $(OBJS) $(O)/i_main.o \
	-o $(O)/linuxxdoom $(LIBS)

$(O)/BlockMap.o: BlockMap.cc BlockMap.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c BlockMap.cc -o $(O)/BlockMap.o

$(O)/BspNode.o: BspNode.cc BspNode.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c BspNode.cc -o $(O)/BspNode.o

$(O)/DivLine.o: DivLine.cc DivLine.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c DivLine.cc -o $(O)/DivLine.o

$(O)/Mob.o: Mob.cc Mob.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c Mob.cc -o $(O)/Mob.o

$(O)/Angle.o: Angle.cc Angle.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c Angle.cc -o $(O)/Angle.o

$(O)/ImageScaler.o: ImageScaler.cc ImageScaler.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c ImageScaler.cc -o $(O)/ImageScaler.o

$(O)/MapThing.o: MapThing.cc MapThing.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c MapThing.cc -o $(O)/MapThing.o

$(O)/DataInput.o: DataInput.cc DataInput.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c DataInput.cc -o $(O)/DataInput.o

$(O)/Vertex.o: Vertex.cc Vertex.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c Vertex.cc -o $(O)/Vertex.o

$(O)/Line.o: Line.cc Line.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c Line.cc -o $(O)/Line.o

$(O)/Sector.o: Sector.cc Sector.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c Sector.cc -o $(O)/Sector.o

$(O)/SubSector.o: SubSector.cc SubSector.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c SubSector.cc -o $(O)/SubSector.o

$(O)/Side.o: Side.cc Side.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c Side.cc -o $(O)/Side.o

$(O)/Seg.o: Seg.cc Seg.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c Seg.cc -o $(O)/Seg.o

$(O)/BoundingBox.o: BoundingBox.cc BoundingBox.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c BoundingBox.cc -o $(O)/BoundingBox.o

$(O)/SpriteFrame.o: SpriteFrame.cc SpriteFrame.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c SpriteFrame.cc -o $(O)/SpriteFrame.o

$(O)/AnimatedSprite.o: AnimatedSprite.cc AnimatedSprite.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c AnimatedSprite.cc -o $(O)/AnimatedSprite.o

$(O)/Sprite.o: Sprite.cc Sprite.hh
	$(CC) $(CFLAGS) $(LDFLAGS) -c Sprite.cc -o $(O)/Sprite.o


$(O)/%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

tags :
	find . -name "*.[chCH]" -print | etags -	

#############################################################
#
#############################################################
