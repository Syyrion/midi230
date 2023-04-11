A tool to convert MIDI files to [30 dollar website](https://thirtydollar.website) files.

Compile the program using your favorite C compiler. This program is just a single C file.

# Usage

Convert a midi file using default settings:
```
midi230 example.mid
```

Specify the output filename:
```
midi230 -o out.moai example.mid
```

Show midi file info:
```
midi230 -i example.mid
```

## Output Configuration
To customize the output, first create a template config file from a midi file:
```
midi230 -t example.mid
```
This will output a file called `example.conf` that will look something like this: 
```conf
# Each line corresponds to each available channel in order.
# ! Every line must be fully filled to prevent unexpected behavior !
# ! i.e. There must be something before and after every comma !
# ! The last line must end with a newline character !

# Keyword, Base pitch, Max volume, Cutoff Velocity    # <Ch#>    Notes     Program
# <Trk1>
# <Trk2>
noteblock_harp, 66, 100, 0    # <Ch0>       234    Flute
noteblock_harp, 66, 100, 0    # <Ch6>       433    Flute
# <Trk3>
noteblock_harp, 66, 100, 0    # <Ch3>       488    Clarinet
# <Trk4>
noteblock_harp, 66, 100, 0    # <Ch0>       354    Trumpet
noteblock_harp, 66, 100, 0    # <Ch1>       506    Muted Trumpet
noteblock_harp, 66, 100, 0    # <Ch5>       527    Muted Trumpet
# <Trk5>
noteblock_harp, 66, 100, 0    # <Ch2>      1524    Bright Acoustic Piano
```
Here you can edit these values to configure the output behavior. Each channel gets its own independent configuration from each line in the order that they appear.

- Keyword
  - The keyword that determines which sound is played for the corresponding channel. All recognized keywords are provided in order in the Appendix.
- Base Pitch
  - The base pitch of a sound from the website with no modifications as a midi pitch. For example, the noteblocks play an F#4 when not pitch-shifted, so the corresponding midi pitch would be 66. A table of midi pitches is provided in the Appendix.
- Max Volume
  - The maximum volume that a sound can take. A midi note with the max velocity of 127 will be played at this volume. (The maximum volume is 600.)
- Cutoff Velocity
  - A midi note's velocity must be greater than this value to be included in the output. Used to exclude notes that are silent or not loud enough to hear. Set to the maximum velocity of 127 to exclude all notes to turn off a channel.

Once done editing the config file, use it by passing it in as an argument:
```
midi230 -c example.conf example.mid
```
# Appendix

## Table of MIDI Pitches
|*Octave*|C|C#|D|D#|E|F|F#|G|G#|A|A#|B
|-|-|-|-|-|-|-|-|-|-|-|-|-
*-1*|0|1|2|3|4|5|6|7|8|9|10|11
*0*|12|13|14|15|16|17|18|19|20|21|22|23
*1*|24|25|26|27|28|29|30|31|32|33|34|35
*2*|36|37|38|39|40|41|42|43|44|45|46|47
*3*|48|49|50|51|52|53|54|55|56|57|58|59
*4*|60*|61|62|63|64|65|66**|67|68|69|70|71
*5*|72|73|74|75|76|77|78|79|80|81|82|83
*6*|84|85|86|87|88|89|90|91|92|93|94|95
*7*|96|97|98|99|100|101|102|103|104|105|106|107
*8*|108|109|110|111|112|113|114|115|116|117|118|119
*9*|120|121|122|123|124|125|126|127|

\* Middle C  
\*\* Default offset (based on the minecraft note blocks)

## Table of Keywords

> **Note:** `_pause` cannot be used as a keyword. To disable a channel, set its cutoff velocity to 127.

||0|1|2|3|4|5|6|7|8|9|10|11|
|-|-|-|-|-|-|-|-|-|-|-|-|-
A|*~~\_pause~~*|boom|bruh|bong|💀|👏|🐶|👽|🔔|💢|💨|🚫
B|💰|🏏|🤬|🚨|buzzer|🅰|e|eight|🍕|🐡|🦆|🦢
C|📲|🌄|hitmarker|👌|🖐|🦀|🚬|whatsapp|😱|❗|slip|explosion
D|🎉|pan|💿|airhorn|taiko_don|taiko_ka|🎹|robtopphone|🎻|🎸|hoenn|🎺
E|gun|dodgeball|whipcrack|gnome|nope|mrbeast|obama|op|SLAM|stopposting|21|americano
F|oof|subaluwa|necoarc|samurai|flipnote|familyguy|pingas|yoda|hehehehaw|ultrainstinct|granddad|morshu
G|smw_coin|smw_1up|smw_spinjump|smw_stomp2|smw_kick|smw_stomp|yahoo|sm64_hurt|thwomp|bup|sm64_painting|smm_scream
H|mariopaint_mario|mariopaint_luigi|smw_yoshi|mariopaint_star|mariopaint_flower|mariopaint_gameboy|mariopaint_dog|mariopaint_cat|mariopaint_swan|mariopaint_baby|mariopaint_plane|mariopaint_car
I|shaker|🥁|hammer|🪘|sidestick|ride2|buttonpop|skipshot|otto_on|otto_off|otto_happy|otto_stress
J|tab_sounds|tab_rows|tab_actions|tab_decorations|tab_rooms|preecho|tonk|rdclap|rdmistake|midspin|adofai_fire|adofai_ice
K|adofaikick|adofaicymbal|cowbell|karateman_throw|karateman_offbeat|karateman_hit@-3|karateman_bulb|ook|choruskid|builttoscale|perfectfail|🌟
L|fnf_left|fnf_down|fnf_up|fnf_right|fnf_death|gdcrash|gdcrash_orbs|gd_coin|gd_orbs|gd_diamonds|gd_quit|bwomp
M|undertale_hit|undertale_crack|sans_voice|megalovania|🦴|undertale_encounter|toby|gaster|lancersplat|isaac_hurt|isaac_dead|isaac_mantle
N|BABA|YOU|DEFEAT|vvvvvv_flip|vvvvvv_hurt|vvvvvv_checkpoint|vvvvvv_flash|terraria_star|terraria_pot|terraria_reforge|terraria_guitar|terraria_axe
O|celeste_dash|celeste_death|celeste_spring|celeste_diamond|amogus_emergency|amogus_kill|amongus|amongdrip|amogus|minecraft_explosion|minecraft_anvil|minecraft_bell
P|noteblock_harp|noteblock_bass|noteblock_snare|noteblock_click|noteblock_bell|noteblock_banjo|noteblock_bit|noteblock_chime|noteblock_xylophone|noteblock_guitar|noteblock_flute|noteblock_pling