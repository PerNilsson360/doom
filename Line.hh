#ifndef _LINE_HH_
#define _LINE_HH_

/*
  [4-3]: LINEDEFS
  ===============

  Each linedef represents a line from one of the VERTEXES to another,
  and each linedef's record is 14 bytes, containing 7 <short> fields:

  (1) from the VERTEX with this number (the first vertex is 0).
  (2) to the VERTEX with this number (31 is the 32nd vertex).
  (3) flags, see [4-3-1] below.
  (4) types, see [4-3-2] below.
  (5) is a "tag" or "trigger" number which ties this line's effect type
  to all SECTORS that have the same tag number (in their last
  field).
  (6) number of the "right" SIDEDEF for this linedef.
  (7) "left" SIDEDEF, if this line adjoins 2 SECTORS. Otherwise, it is
  equal to -1 (FFFF hex).

  "right" and "left" are based on the direction of the linedef as
  indicated by the "from" and "to", or "start" and "end", VERTEXES.
  This sketch should make it clear:

  left side               right side
  start -----------------> end <----------------- start
  right side              left side

  IMPORTANT: All lines must have a right side. If it is a one-sided
  line, then it must go the proper direction, so its single side is
  facing the sector it is part of. DOOM will crash on a level that has
  a line with no right side.


  [4-3-1]: Linedef Flags
  ----------------------

  The third <short> field of each linedef controls some attributes of
  that line. These attributes (aka flags) are indicated by bits. If the
  bit is set (equal to 1), the condition is true for that line. If the
  bit is not set (equal to 0), the condition is not true. Note that the
  "unpegged" flags cannot be independently set for the two SIDEDEFs of
  a line. Here's a list of the flags, followed by a discussion of each:

  bit     Condition

  0       Impassible
  1       Block Monsters
  2       Two-sided
  3       Upper Unpegged
  4       Lower Unpegged
  5       Secret
  6       Block Sound
  7       Not on Map
  8       Already on Map
  9-15    unused

  0 (Impassible) - Players and monsters cannot cross this line. Note that
  if there is no sector on the other side, they can't go through the line
  anyway, regardless of the flags.

  1 (Block Monsters) - Monsters cannot cross this line.

  2 (Two-sided) - The linedef's two sidedefs can have "-" as a texture,
  which in this case means "transparent". If this flag is not set, the
  sidedefs can't be transparent: if "-" is viewed, it will result in the
  "hall of mirrors" effect. However, a linedef CAN have two non-transparent
  sidedefs, even if this flag is not set, as long as it is between two
  sectors.
  Another side effect of this flag is that if it is set, then gunfire
  (pistol, shotgun, chaingun) can go through it. If this flag is not set,
  gunfire cannot go through the line. Projectiles (rockets, plasma etc.)
  are not restricted this way. They can go through as long as there's a
  sector on the other side (and the sector heights allow it).
  Finally, monsters can see through and attack through two-sided lines,
  despite any of the line's other flag settings and textures (once again,
  provided sector heights and the REJECT [4-10] allow it).

  3 (Upper unpegged) - The upper texture is pasted onto the wall from
  the top down instead of from the bottom up like usual.
  Upper textures normally have the bottom of the wall texture to be
  drawn lined up with the bottom of the "upper" space in which it is
  to be drawn (sidedef Y offsets then apply [4-4]). This can result
  in the upper texture being misaligned with a neighboring "middle"
  texture. To help solve this problem, common at "windows", this flag
  can be set.
  If the upper texture is unpegged, it is drawn with the wall texture's
  top row at the ceiling, just like middle and lower textures are usually
  drawn. This can help realign the upper texture with a neighbor.

  The article TEXTURES, cited in appendix [A-4] gives a great deal
  more explanation on the "unpegged" flags and how to use them.

  4 (Lower unpegged) - Lower and middle textures are drawn from the
  bottom up, instead of from the top down like usual.
  This is also commonly used on lower textures under "windows". It is
  also used on doorjambs, because when the door opens, the sector ceiling
  is rising, so the "sides" (the doorjambs), which are middle textures,
  will be drawn from the ever-changing ceiling height down, and thus will
  appear to be "moving". Unpegging them will make them be drawn from the
  floor up, and since the floor height doesn't change when a door opens,
  then will not move.
  There's one slight difference with lower textures being unpegged -
  they are not necessarily drawn with the bottom of the wall texture placed
  at the bottom of the wall. The height of the facing sector and the height
  of the wall texture are taken into account. So if the sector is 160 high,
  and the wall texture is 128 high, then lower unpegged will cause the 32nd
  row of the wall texture to be at the floor, NOT the 128th row. This of
  course excludes sidedef Y offsets, which are applied AFTER unpegged
  flags do their stuff.

  5 (Secret) - On the automap, this line appears in red like a normal
  solid wall that has nothing on the other side. This is useful in
  protecting secret doors and such. Note that if the sector on the other
  side of this "secret" line has its floor height HIGHER than the sector
  on the facing side of the secret line, then the map will show the lines
  beyond and thus give up the secret.
  Also note that this flag has NOTHING to do with the SECRETS ratio on
  inter-level screens. That's done with special sector 9 (see [4-9-1]).

  6 (Block Sound) - For purposes of monsters hearing sounds and thus
  becoming alerted. Every time a player fires a weapon, the "sound" of
  it travels from sector to sector, alerting all non-deaf monsters in
  each new sector. But the sound will die when it hits a second line
  with this flag. The sound can cross one such line, but not two. All
  possible routes for the sound to take are taken, so it can get to
  some out-of-the-way places. Another thing that blocks sound, instantly,
  is incompatible sector heights. Sound can go from a sector with 0/72
  floor/ceiling heights to one with 64/192, but the sound CANNOT go
  from a 0/128 sector to an adjacent 128/256 sector.

  7 (Not on Map) - The line is not shown on the automap, even if the
  computer all-map power up is acquired.

  8 (Already on Map) - When the level is begun, this line is already
  on the automap, even though it hasn't been seen (in the display) yet.
  Normally lines only get mapped once part of the line has been seen in
  the display window.

  Automap line colors: Red lines indicate the line is one-sided, that
  there is a sector on only one side (or the line is marked secret).
  Brown lines are between two sectors with different floor heights but
  the same ceiling height. Yellow lines are between two sectors with
  different ceiling heights and the same or different floor heights.
  Gray lines are as-yet-unseen lines revealed by the computer all-map.
  Without the all-map, lines between sectors with identical floor and
  ceiling heights don't show up. With it, they are gray.


  [4-3-2]: Linedef Types
  ----------------------

  The <short> in field 4 of 7 of a linedef can control various special
  effects like doors opening, floors moving, etc. Some of them must be
  activated by "using" them, like switches, and some of them are activated
  when they are walked over. There are a huge number of ways to use these
  effects, but it's all done by using one of a hundred or so line function
  types.
  The most common way they work is this: a player walks across a line
  or activates (presses the spacebar or the use key) right in front of
  a line. That line has a function type that is non-zero. It also has
  a tag number. Then ALL sectors on the level with the same tag number,
  that are not already engaged in some action, undergo the effects that
  the linedef type number dictates. Note that the tag numbers are NOT the
  sector numbers, nor the linedef numbers. A tag number is in a lindef's
  5th <short> field, and a sector's last <short> field.

  Explanations of all the abbreviations in the table:

  Val     The value of the linedef "type" field (#4). If you want them
  in numerical order, use SORT or something.
  *       This line function only works in 1.666 and up
  Class   The category of the effect
  Act     Activation. How the linedef's effect is activated.
  n       does NOT require a tag number (see note 5 below)
  W       walk-over activation
  S       switch ("use" - default config is spacebar)
  G       gunfire (pistol, shotgun, chaingun) cross or hit line
  1       the line may be activated once only
  R       potentially repeatable activation
  &       affected sectors "locked out" from further changes. See notes 9/10.
  m       Monster actions can activate the line's effect
  Sound   The type of noise made by moving sectors
  Speed   How quickly a floor moves up/down etc.
  Tm      Time - how long it "rests"; doors "rest" when they've gone as 
  high as they're going to go, lifts "rest" at the bottom, etc.
  Chg     Change - some of them cause a floor texture change and/or special
  sector change. See note 11 below.
  T       Trigger model, see note 11 below.
  N       Numeric model, see note 11 below.
  X       Floor texture is transferred, and Sector type 0.
  P       Special Sector types 4, 5, 7, 9, 11, 16 transfer.
  Effect  What happens to the affected sector(s).
  open    The ceiling goes (up) to LEC-4.
  close   The ceiling goes (down) to the floor level.
  up      Will move up at specified speed if the destination is above.
  If the destination is below, it arrives there instantly.
  down    Will move down at specified speed if the destination is below.
  If the destination is above, it arrives there instantly.
  open/   The door can be activated while moving. If it's open or opening,
  close   it closes. If it's closed or closing, it opens, then pauses,
  then closes.
  open,   The door can only be activated if it is in the closed state.
  close   It opens, pauses, then closes.
  lift    The floor goes down to LIF, rests, then back up to original height.
  L       lowest
  H       highest
  C       ceiling
  F       floor
  E       adjacent sectors, excluding the affected sector
  I       adjacent sectors, including the affected sector
  nh      next-higher, i.e. LEF that is higher than source.

  More notes and longer discussions related to these terms:

  1.  "Adjacent" is any sector that shares a LINEDEF with the tagged sector
  (sectors are adjacent to themselves).
  2.  All S activations and the teleporters only work from the RIGHT side
  of the linedef.
  3.  For teleporters, if more than 1 sector is tagged the same and each
  has a teleport landing THING, then the lowest numbered sector is the
  destination.
  4.  Floors that raise up an absolute height (up 24, 32) will go up INTO
  ceilings, so using the WR and SR types of these in levels is unwise.
  5.  A few of the linedef types don't require tag numbers: the end-level
  switches, the scrolling wall type 48 (0x30), and the "manual" doors which
  operate on the sector facing the left side of the linedef with the manual
  door line type.
  666.  Here's the terms id uses for different types of activations:
  Manual: nSR and nS1 doors
  Trigger: W1
  Retrigger: WR
  Switch: S1
  Button: SR
  Impact: G
  Effect: line 48 is the only one
  7.  The "moving floors" go up to a maximum of HIF and go down to a minimum
  of LIF. Why they sometimes go up first and sometimes down is still a
  mystery to me.
  8.  The "crushing ceilings" go from their original ceiling height down
  to (floor + 8), then back up. While crushing a creature, their downward
  speed slows considerably. "Fast hurt" does about 120% total damage per
  crush, and "slow hurt" grabs you and does somewhere around 1000-2000%
  total damage per crush.
  9.  The & symbol indicates that a sector cannot be affected any more by
  other line types after it has performed this effect, even if it has
  finished. These are the floor-texture-changers and... (keep reading)
  10. Moving floors and crushing ceilings also "lock out" further changes
  to the sectors affected, EXCEPT for restarting the moving floor or
  crushing ceiling. If a line triggers a type 6 crushing ceiling in a
  sector, then it is stopped, then ANY other line with a "crush" type that
  is tagged to the same sector will cause the type 6 crusher to start
  again, with its original maximum and minimum ceiling heights.
  11. Some line types cause floor textures and/or some special sector types
  (see [4-9-1]) to transfer to the tagged sector. The tagged sectors' floor
  and/or special sector (SS) type will change to match that of the "model"
  sector. The TRIGGER model gets the info from the sector that the
  activating line's right side faces. The NUMERIC model gets the info
  from the sector on the other side of the lowest numbered two-sided
  linedef that has one side in the tagged sector.
  All of these "change" line types transfer the floor texture. Also,
  they all can pass a special sector trait of "0" or "nothing", i.e. if
  the destination is an acid-floor or "damaging" sector, then any of these
  lines can erase the damaging effect. Lines 59, 93, 37, 84, and 9 (see
  note 12 for more specifics on line type 9) also have the ability to 
  transfer the "secret" trait of SS 9, and the damaging traits of SS 
  4, 5, 7, 11, and 16. None of the "blinking light" effects of SSs can be
  transferred. SS 4 "blinks" and causes damage, but only the damaging part
  can be transferred. SS 11 also turns off god-mode and causes a level END
  when health <11%, this characteristic is part of SS 11, and cannot be
  isolated via fancy transfers.
  12. Line type 9 is a special one. The definitive example is the chainsaw
  pillar on E1M2. Take the lowest-numbered linedef that has a sidedef in
  the tagged sector. If that linedef is one-sided, nothing happens. If it
  is 2-sided, then the tagged sector's floor will move down to match the
  2nd sector's floor height (or it will jump instantly up if it was below,
  like other floors that are supposed to move "down").
  If this 2nd sector CONTAINS the tagged sector, i.e. all the linedefs
  with a sidedef in the tagged sector have their other sidedef in the 2nd
  sector, then this 2nd sector is the "donut". This donut's floor will
  move "up" to match the floor height of the sector on the other side of
  the DONUT's lowest-numbered linedef, excluding those linedefs that are
  shared with the "donut hole" central sector. Also, the donut will undergo
  a floor texture change and special sector type change to match the
  "outside". The donut sector does not have to be completely surrounded
  by another sector (i.e. it can have 1-sided linedefs), but if its
  lowest-numbered linedef is not 2-sided, a minor glitch results: the donut
  and the donut-hole both move to a strange height, and the donut changes
  floor texture to TLITE6_6 - the last flat in the directory.
  Note that if the donut hole and the donut are both going to move, the
  donut hole is going to move to match the height that the donut is "going
  to". In other words, the whole thing will be at a single height when
  they're done, and this is the height of the "outside" sector that borders
  the donut.
  Whew!
  13. Line types 30 and 96, "up ShortestLowerTexture" means that affected
  sector(s) floors go up a number of units equal to the height of the
  shortest "lower" texture facing out from the sector(s).
  14. STAIRS. Any sector tagged to a stair-raiser line will go up 8. Now
  find the lowest-numbered 2-sided linedef whose RIGHT side faces this
  sector (the first step). The sector on the other side of the lindedef
  becomes the next step, and its floor height changes to be 8 above the
  previous step (it raises up if it was lower, or it changes instantly if
  it was higher). This process continues as long as there are 2-sided
  lines with right sides facing each successive step. A couple things
  will stop the stairway:
  (a) no 2-sided linedef whose right side faces the current step
  (b) a sector with a different floor texture
  (c) a sector that has already been moved by this stairway (this stops
  ouroboros stairways that circle around to repeat themselves)
  (d) "locked-out" sectors that can't change their floor height anymore
  The component steps of a stairway can have any shape or size.
  The turbo stairs (100, 127) work just like regular stairs except that
  each step goes up 16 not 8, and rising steps can crush things between
  themselves and the ceiling.
  15. Line types 78 and 85 do NOTHING as of version 1.666.



  Val   Class Act  Sound Speed Tm Chg Effect

  SPECIAL (Continuous effect, doesn't need triggereing)

  48   Spec  n--  -     -     -  -   Scrolling wall

  LOCAL DOORS ("MANUAL" DOORS)

  1   mDoor nSRm door  med   4  -   open/close
  26   mDoor nSR  door  med   4  -   open/close BLUE KEY
  28   mDoor nSR  door  med   4  -   open/close RED KEY
  27   mDoor nSR  door  med   4  -   open/close YELLOW KEY
  31   mDoor nS1  door  med   -  -   open
  32   mDoor nS1  door  med   -  -   open BLUE KEY
  33   mDoor nS1  door  med   -  -   open RED KEY
  34   mDoor nS1  door  med   -  -   open YELLOW KEY
  46   mDoor nGR  door  med   -  -   open
  117 * mDoor nSR  blaze turbo 4  -   open/close
  118 * mDoor nS1  blaze turbo -  -   open

  REMOTE DOORS

  4   rDoor  W1  door  med   4  -   open,close
  29   rDoor  S1  door  med   4  -   open,close
  90   rDoor  WR  door  med   4  -   open,close
  63   rDoor  SR  door  med   4  -   open,close
  2   rDoor  W1  door  med   -  -   open
  103   rDoor  S1  door  med   -  -   open
  86   rDoor  WR  door  med   -  -   open
  61   rDoor  SR  door  med   -  -   open
  3   rDoor  W1  door  med   -  -   close
  50   rDoor  S1  door  med   -  -   close
  75   rDoor  WR  door  med   -  -   close
  42   rDoor  SR  door  med   -  -   close
  16   rDoor  W1  door  med   30 -   close, then opens
  76   rDoor  WR  door  med   30 -   close, then opens
  108 * rDoor  W1  blaze turbo 4  -   open,close
  111 * rDoor  WR  blaze turbo 4  -   open,close
  105 * rDoor  S1  blaze turbo 4  -   open,close
  114 * rDoor  SR  blaze turbo 4  -   open,close
  109 * rDoor  W1  blaze turbo -  -   open
  112 * rDoor  S1  blaze turbo -  -   open
  106 * rDoor  WR  blaze turbo -  -   open
  115 * rDoor  SR  blaze turbo -  -   open
  110 * rDoor  W1  blaze turbo -  -   close
  113 * rDoor  S1  blaze turbo -  -   close
  107 * rDoor  WR  blaze turbo -  -   close
  116 * rDoor  SR  blaze turbo -  -   close
  133 * rDoor  S1  blaze turbo -  -   open BLUE KEY
  99 * rDoor  SR  blaze turbo -  -   open BLUE KEY
  135 * rDoor  S1  blaze turbo -  -   open RED KEY
  134 * rDoor  SR  blaze turbo -  -   open RED KEY
  137 * rDoor  S1  blaze turbo -  -   open YELLOW KEY
  136 * rDoor  SR  blaze turbo -  -   open YELLOW KEY

  CEILINGS

  40   Ceil   W1  mover slow  -  -   up to HEC
  41   Ceil   S1  mover slow  -  -   down to floor
  43   Ceil   SR  mover slow  -  -   down to floor
  44   Ceil   W1  mover slow  -  -   down to floor + 8
  49   Ceil   S1  mover slow  -  -   down to floor + 8
  72   Ceil   WR  mover slow  -  -   down to floor + 8

  LIFTS

  10   Lift   W1  lift  fast  3  -   lift
  21   Lift   S1  lift  fast  3  -   lift
  88   Lift   WRm lift  fast  3  -   lift
  62   Lift   SR  lift  fast  3  -   lift
  121 * Lift   W1  lift  turbo 3  -   lift
  122 * Lift   S1  lift  turbo 3  -   lift
  120 * Lift   WR  lift  turbo 3  -   lift
  123 * Lift   SR  lift  turbo 3  -   lift

  FLOORS

  119 * Floor  W1  mover slow  -  -   up to nhEF
  128 * Floor  WR  mover slow  -  -   up to nhEF
  18   Floor  S1  mover slow  -  -   up to nhEF
  69   Floor  SR  mover slow  -  -   up to nhEF
  22   Floor  W1& mover slow  -  TX  up to nhEF
  95   Floor  WR& mover slow  -  TX  up to nhEF
  20   Floor  S1& mover slow  -  TX  up to nhEF
  68   Floor  SR& mover slow  -  TX  up to nhEF
  47   Floor  G1& mover slow  -  TX  up to nhEF
  5   Floor  W1  mover slow  -  -   up to LIC
  91   Floor  WR  mover slow  -  -   up to LIC
  101   Floor  S1  mover slow  -  -   up to LIC
  64   Floor  SR  mover slow  -  -   up to LIC
  24   Floor  G1  mover slow  -  -   up to LIC
  130 * Floor  W1  mover turbo -  -   up to nhEF
  131 * Floor  S1  mover turbo -  -   up to nhEF
  129 * Floor  WR  mover turbo -  -   up to nhEF
  132 * Floor  SR  mover turbo -  -   up to nhEF
  56   Floor  W1& mover slow  -  -   up to LIC - 8, CRUSH
  94   Floor  WR& mover slow  -  -   up to LIC - 8, CRUSH
  55   Floor  S1  mover slow  -  -   up to LIC - 8, CRUSH
  65   Floor  SR  mover slow  -  -   up to LIC - 8, CRUSH
  58   Floor  W1  mover slow  -  -   up 24
  92   Floor  WR  mover slow  -  -   up 24
  15   Floor  S1& mover slow  -  TX  up 24
  66   Floor  SR& mover slow  -  TX  up 24
  59   Floor  W1& mover slow  -  TXP up 24
  93   Floor  WR& mover slow  -  TXP up 24
  14   Floor  S1& mover slow  -  TX  up 32
  67   Floor  SR& mover slow  -  TX  up 32
  140 * Floor  S1  mover med   -  -   up 512
  30   Floor  W1  mover slow  -  -   up ShortestLowerTexture
  96   Floor  WR  mover slow  -  -   up ShortestLowerTexture
  38   Floor  W1  mover slow  -  -   down to LEF
  23   Floor  S1  mover slow  -  -   down to LEF
  82   Floor  WR  mover slow  -  -   down to LEF
  60   Floor  SR  mover slow  -  -   down to LEF
  37   Floor  W1  mover slow  -  NXP down to LEF
  84   Floor  WR  mover slow  -  NXP down to LEF
  19   Floor  W1  mover slow  -  -   down to HEF
  102   Floor  S1  mover slow  -  -   down to HEF
  83   Floor  WR  mover slow  -  -   down to HEF
  45   Floor  SR  mover slow  -  -   down to HEF
  36   Floor  W1  mover fast  -  -   down to HEF + 8
  71   Floor  S1  mover fast  -  -   down to HEF + 8
  98   Floor  WR  mover fast  -  -   down to HEF + 8
  70   Floor  SR  mover fast  -  -   down to HEF + 8
  9   Floor  S1  mover slow  -  NXP donut (see note 12 above)

  STAIRS

  8   Stair  W1  mover slow  -  -   stairs
  7   Stair  S1  mover slow  -  -   stairs
  100 * Stair  W1  mover turbo -  -   stairs (each up 16 not 8) + crush
  127 * Stair  S1  mover turbo -  -   stairs (each up 16 not 8) + crush

  MOVING FLOORS

  53   MvFlr  W1& lift  slow  3  -   start moving floor
  54   MvFlr  W1& -     -     -  -   stop moving floor
  87   MvFlr  WR& lift  slow  3  -   start moving floor
  89   MvFlr  WR& -     -     -  -   stop moving floor

  CRUSHING CEILINGS

  6   Crush  W1& crush med   0  -   start crushing, fast hurt
  25   Crush  W1& crush med   0  -   start crushing, slow hurt
  73   Crush  WR& crush slow  0  -   start crushing, slow hurt
  77   Crush  WR& crush med   0  -   start crushing, fast hurt
  57   Crush  W1& -     -     -  -   stop crush
  74   Crush  WR& -     -     -  -   stop crush
  141 * Crush  W1& none? slow  0  -   start crushing, slow hurt "Silent"

  EXIT LEVEL

  11   Exit  nS-  clunk -     -  -   End level, go to next level
  51   Exit  nS-  clunk -     -  -   End level, go to secret level
  52   Exit  nW-  clunk -     -  -   End level, go to next level
  124 * Exit  nW-  clunk -     -  -   End level, go to secret level

  TELEPORT
 
  39   Telpt  W1m tport -     -  -   Teleport
  97   Telpt  WRm tport -     -  -   Teleport
  125 * Telpt  W1m tport -     -  -   Teleport monsters only
  126 * Telpt  WRm tport -     -  -   Teleport monsters only

  LIGHT

  35   Light  W1  -     -     -  -   0
  104   Light  W1  -     -     -  -   LE (light level)
  12   Light  W1  -     -     -  -   HE (light level)
  13   Light  W1  -     -     -  -   255
  79   Light  WR  -     -     -  -   0
  80   Light  WR  -     -     -  -   HE (light level)
  81   Light  WR  -     -     -  -   255
  17   Light  W1  -     -     -  -   Light blinks (see [4-9-1] type 3)
  138 * Light  SR  clunk -     -  -   255
  139 * Light  SR  clunk -     -  -   0
*/

#include "Vertex.hh"
#include "BoundingBox.hh"
#include "r_defs.h"

// Solid, is an obstacle.
#define ML_BLOCKING		1

// Blocks monsters only.
#define ML_BLOCKMONSTERS	2

// Backside will not be present at all
//  if not two sided.
#define ML_TWOSIDED		4

// If a texture is pegged, the texture will have
// the end exposed to air held constant at the
// top or bottom of the texture (stairs or pulled
// down things) and will move with a height change
// of one of the neighbor sectors.
// Unpegged textures allways have the first row of
// the texture at the top pixel of the line for both
// top and bottom textures (use next to windows).

// upper texture unpegged
#define ML_DONTPEGTOP		8

// lower texture unpegged
#define ML_DONTPEGBOTTOM	16	

// In AutoMap: don't map as two sided: IT'S A SECRET!
#define ML_SECRET		32

// Sound rendering: don't let sound cross two of these.
#define ML_SOUNDBLOCK		64

// Don't draw on the automap at all.
#define ML_DONTDRAW		128

// Set if already seen, thus drawn in automap.
#define ML_MAPPED		256

class DataInput;
class Sector;

class Line
{
public:
    enum SlopeType {
	ST_HORIZONTAL,
	ST_VERTICAL,
	ST_POSITIVE,
	ST_NEGATIVE
    };
    Line();
    Line(const DataInput& datainput,
	 std::vector<Vertex>& vertices,
	 std::vector<Side>& sides);
    static int getBinarySize() { return 7 * 2; }
    int boxOnLineSide(const BoundingBox& box);
    
//private:
    // Vertices, from v1 to v2.
    Vertex*	v1;
    Vertex*	v2;

    // Precalculated v2 - v1 for side checking.
    double ddx;
    double ddy;

    // Animation related.
    short	flags;
    short	special;
    short	tag;

    // Visual appearance: SideDefs.
    //  sidenum[1] will be -1 if one sided
    short	sidenum[2];			

    // Neat. Another bounding box, for the extent
    //  of the LineDef.
    double bbbox[4];
    BoundingBox box;
    
    // To aid move clipping.
    SlopeType	slopetype;

    // Front and back sector.
    // Note: redundant? Can be retrieved from SideDefs.
    Sector*	frontsector;
    Sector*	backsector;

    // if == validcount, already checked
    int		validcount;

    // thinker_t for reversable actions
    void*	specialdata;		
};

#endif
