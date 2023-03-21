#Tile info patching script for Keen 3
%ext ck3

%version 1.0
%patchfile 0x199F8 simkeen.tli

%version 1.31
%patchfile 0x198C8 simkeen.tli

#####################################################
#########     General Gameplay ######################
#####################################################

### infinite lives###

%patch $8E00 $90 $90 $90 $90

#Reset score on death A
%patch $7928 $66 $81 $06 $0E $71 $0800W $0000W  $66 $A1 $AF00W  $66 $A3 $9D4AW  $90 $90

#Reset score on death B
%patch $7965 $66 $81 $06 $0E $71 $0800W $0000W  $66 $A1 $AF00W  $66 $A3 $9D4AW  $90 $90

#Save score on level load
%patch $81B7 $66 $A1 $9D4AW  $66 $A3 $AF00W  $66 $33 $C0 $66 $A3 $5361W  $66
             $A3 $586AW  $A3 $5D40W  $90

### exit ###

#Don't draw exit tiles
%patch $77E6 $90 $90 $90 #Top left tile
%patch $7810 $90 $90 $90 #Bottom left tile
%patch $783A $90 $90 $90 #Top right tile
%patch $786A $90 $90 $90 #Bottom right tile

#Length of exit walk
%patch $7B28 $90

#What raygun gives Keen
%patch $7ACD $9D48W  $02

%patch $8B31 $004DW #Small marker
%patch $8BAA $004EW #Big marker top left
%patch $8BC3 $004FW #Big marker top right
%patch $8BDA $0050W #Big marker bottom left
%patch $8BF4 $0051W #Big marker bottom right

%patch $B990 $0152W #Start of animation sequence at teleporter Keen starts at
%patch $BA03 $0156W #End of animation sequence at teleporter Keen starts at
%patch $B9F9 $0004W #Animation speed

%patch $BAF4 $0152W #Start of animation sequence at teleporter Keen arrives at
%patch $BB67 $0156W #End of animation sequence at teleporter Keen arrives at
%patch $BB5D $0004W #Animation speed

%patch $BA45 $0063W #Tile appearing at teleport Keen left (after animation)
%patch $BBA9 $0063W #Tile appearing at teleport Keen arrives at (after animation)

#####################################################
#########     Sprites      ##########################
#####################################################

### vorticon ###

#Vorticon sprite type:
%patch $3C2B $05 #vorticon pushes
#Jump height
%patch $418D $009CW #yorp jump height

### meep ###

#Create new randomizer for MEEP
#SIDE EFFECT: Unknown behavior not used
%patch $7F03 $C3 $8B $1E $83 $53 $43 $81 $E3 $01 $00     #Frequency is $3F, smaller = more often
             $89 $1E $83 $53 $8A $87 $85 $53 $32 $E4 $C3

#Meep uses new randomizer (Anything else can use it too!)
%patch $4774 $8E $37

#Meep disappear after shooting:
%patch $4893 $05 $7E #Disappear

### Jack ###

%patchfile $636C cg3.bin
%patchfile $65A8 homejack.bin

%patch $3F22 $A8 $65
%patch $3CB0 $05 $7E

%patch $3F27 $98 $48 #Jack spawns dead Meep

### vortimom ###

%patch $456D $65A8W   #Vortimom's shot is a homing jack
%patch $3D1C $0003W   #Vortimom strength

### Foob ###

#Running runs towards keen
%patch $4B6C $06 $FF 
%patch $4B74 $FA $00

%patch $3E1B $02 $00 #Foob kills
%patch $4B8A $3D $00 #Foob not killed by Keen

### Mangling Arm ###

#Arm tiles
%patch $51E3 $0116W #Arm start tile
%patch $5237 $0116W #Main arm tile (Must match above tile)
%patch $5255 $0116W #Tile between top of two fingers (Must be the same or larger value as two above values)

#Left finger
%patch $526D $0116W #Top
%patch $5286 $0116W #Middle
%patch $52A0 $0113W #Bottom

#Right finger
%patch $52B8 $0116W #Top
%patch $52D1 $0116W #Middle
%patch $52EB $0113W #Bottom

#Stop tile (Floor tile)
%patch $5214 $0117W $77

#Wallpaper tiles that appear when... (Should all be the same)
%patch $5149 $0186W #...top of left finger moves down
%patch $5162 $0186W #Unused?
%patch $517C $0186W #...bottom of left finger moves up
%patch $5131 $0186W #Lifting arm
%patch $5194 $0186W #...top of right finger moves down
%patch $51AD $0186W #Unused?
%patch $51C7 $0186W #...bottom of right finger moves up

############################################################
#############   Text Patches    ############################
############################################################

#Make an external story files, max size 2.9KB
%patchfile $18BC5 "STORY.txt"

#Menu bottom item, remove two options
%patch $9CF6 $05    #Moving down
%patch $9C72 $0005W #When wrapping from top
%patch $9EB9 $05    #When refreshing window

#rearrange the bottom options, to drop "preview" and "ordening info" instead.
%patch $9EDC $9E4FW # New Game
             $9E56W # Continue Game
             $9E5EW # Story
             $9E68W # About ID...
             $9E78W # High Scores
             $9EACW # Ordering Info

#Menu
%patch $1C7D8 "New Game"
%patch $1C7E5 "Continue Game"
%patch $1C7F7 "Story"
%patch $1C801 "Credits    "
%patch $1C811 "High Scores"
%patch $1C821 "Restart Demo "
%patch $1C833 "         "
%patch $1C841 "            "
%patch $1C850 "Use the " $00 "arrows"

#About ID
%patch $1C870 "This mod is made by Shikadi       "
%patch $1C898 "The randomizer is built upon     "
%patch $1C8C0 "the Keen 1 randomise tool     "
%patch $1C8E8 "built by Multimania "
%patch $1C910 "I made none of the graphics     "
%patch $1C938 "myself, They were taken from       "
%patch $1C960 "Itch.io. The ice Assets by"
%patch $1C988 "Coloritmic.pixel object"
%patch $1C9B0 "by untiedgames. Grassland by "
%patch $1C9D8 "Anokolisa. Inca by Krobits. Food "
%patch $1CA00 "by Henry software.       "

#Satus Screen
%patch $1BAF8 "SCORE"
%patch $1BB02 "EXTRA KEEN AT"
%patch $1BB19 "KEENS"
%patch $1BB2A "AMMO  "
%patch $1BB3D "ANKH TIME"
%patch $1BB4C "KEYS     "
%patch $1BB68 "PLEASE PRESS A KEY"
%patch $1C7CA "Keens Left"

#Finale
%patch $1C36D "Suddenly, the world became     "
%patch $1C395 "blurry. As if commander keen was    "
%patch $1C3BB "waking up from a strange dream.       "
%patch $1C3E3 "                                      "

%patch $1C40B "The world he was in just seconds "
%patch $1C433 "ago is gone.                          "
%patch $1C45B "                                     "

%patch $1C482 "He wonders what happened.             "
%patch $1C4AA "                                 "
%patch $1C4D2 "                          "

#Stop textboxs appearing
%patch $5721 $EB $38
%patch $575B $EB $35
%patch $5792 $EB $35
%patch $57C9 $EB $7D
%patch $5848 $EB $7D
%patch $58C7 $EB $47
%patch $5910 $EB $6B
%patch $597D $EB $33
%patch $59B2 $EB $52

#Don't change level to ending level until... just don't at all
%patch $5AA2 $90 $90 $90 #After first textbox
%patch $5AE9 $90 $90 $90 #After second
%patch $5B25 $90 $90 $90 #Third
%patch $5B61 $90 $90 $90 #Fourth
%patch $5BA2 $90 $90 $90 #First flash start
%patch $5BC0 $90 $90 $90 #First flash end
%patch $5BE4 $90 $90 $90 #Second flash start
%patch $5C03 $90 $90 $90 #Second flash end
%patch $5C27 $90 $90 $90 #Third flash start
%patch $5C46 $90 $90 $90 #Third flash end
%patch $5C69 $90 $90 $90 #Fourth flash start
%patch $5C87 $90 $90 $90 #Fourth flash end
%patch $5CAB $90 $90 $90 #Fifth flash start
%patch $5CCA $90 $90 $90 #Fifth flash end (Finale)

#Final text
%patch $1C36D "Huh?                           "
%patch $1C395 "Was it all a dream? A simulation all"
%patch $1C3BB "along? Commander keen feels like he   "
%patch $1C3E3 "woke up from a deep sleep.            "
%patch $1C4EE "Thank you!  "
%patch $5AEC $EB $3A
%patch $5B28 $EB $3A

%end