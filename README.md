# MIDNIGHT MATINEE
A Contra-style run-and-gun horror platformer for Wii U homebrew (Aroma), drawn like a 1930s rubber-hose cartoon.
Built on The Tolling's engine; the two-screen setup follows Hall Pass.

Uncle Inky's Jamboree Palace has been condemned for decades. When the last reel plays, its ink-soaked animatronics
wake up. You're the night crew sent in to shut it down.

## Level 1: Opening Night
Lobby, Auditorium, Boiler Room, Backstage, Projection Booth, then the Main Stage boss fight.
* **Usherettes**: three-armed porcelain dolls that fire flashlight beams.
* **Projector Dogs**: patrol the catwalks, warm up their lens and fire a long beam (jump or duck it).
* **Ink Blobs**: hop at you and split in two when shot.
* **Ink drips** fall from the rafters in the boiler room; **ink floods** fill the pits.
* **The Ink Fiend** (Backstage): can't be killed. When the ink creeps in from the edges, hide in a PROPS closet (Up)
  until it passes, or have the security office hit LIGHTS on CAM 4 to drive it back.
* **Boss: Uncle Inky**, the animatronic octopus bandleader: tracked tentacle slams, bouncing ink notes, a spotlight
  sweep along the floor (jump it), and when he's angry, ticket rain and ink blob reinforcements.

## Level 2: Pizza Party
Party Room, Dining Hall, Pizza Counter, Kitchen, Delivery Dock, then The Rat's Den. Pick the show on the title screen
with Left/Right; clearing a show carries your lives, weapon and score into the next one.
* **Party Goats**: animatronic goats that blast confetti arcs from their accordions.
* **Meatballs**: roll at you and pounce.
* **Flying Slices**: bob through the air and dive-bomb you.
* **Hot cheese pits**, **pizza ovens** that belch fire columns on a timer (LIGHTS in that zone shuts them off), and
  **conveyor belts** that carry you and the enemies; the Delivery Dock crosses a cheese lake on moving belts.
* **Boss: Big Cheese**, the chef who ate the birthday: cleaver chops aimed at you, bouncing pizzas, a cheese wave along
  the floor (jump it) and a leaping body slam with a shockwave. When he's angry he calls in meatballs.

## Gear (Ghosts 'n Goblins style)
Full gear, then torn overalls (the hard hat flies off), then **boxers**, then you're out. GEAR crates put clothes back on.
TOOLS crates drop a cycling weapon icon: grab it when it shows the one you want:
N = Nail Gun, S = Rivet Spread, T = Welding Torch, P = Plunger Launcher (the "bad" weapon).

## Building power
The theatre's power drains over time and the room gets darker. Flip **breaker boxes** (Up) for +40%. At 0% the lights go
out: only a flashlight circle around you, enemy eyes glow, and enemies get faster. The cameras show static.

## The GamePad: Security Office
The GamePad shows its own picture: six live camera feeds of the theatre.
* **ZL / ZR** or **tap** a feed to switch cameras (in solo it follows the action on its own when you leave it alone)
* **PING (-)**, 6% power: marks every enemy on that camera; marked enemies take double damage
* **LIGHTS (X)**, 10% power: floods that zone with light for 8 seconds, dazzles enemies, scares off the Ink Fiend

## Modes
* **Solo**: the GamePad player runs and works the cameras.
* **Security Office co-op**: one player on the GamePad works the cameras, the runner uses a Wii Remote / Pro Controller.
  A second runner can clock in any time with + on another controller.
* **Possessed party mode** (coming soon).

## Controls (runner)
D-pad / stick move and aim (up = diagonal / straight up, down in the air = shoot down) · B jump (Down + B drops through
catwalks) · A or Y shoot · hold R to stand still and aim · Up: hide in a closet / flip a breaker · + start / clock in.

## Building
Push to GitHub: Actions builds `midnight_matinee.wuhb` and publishes it on the **latest** release.
Copy it to `sd:/wiiu/apps/` and launch it from the Wii U Menu with Aroma.

PC test build: `cmake -B build_pc && cmake --build build_pc && ./build_pc/midnight_matinee`
(arrows, Z jump, X shoot, Shift aim-lock, Q/E cameras, W ping, R lights, Enter start; the GamePad screen opens in a
second window and you can click it). Headless tests: see the top of `engine/src/main.c`.

## Art and music
Characters, the boss and the background are Runway paintings on flat green, cut by `tools/cut_sprites.py`
(sources in `art/refs`).

Music style: **haunted theater organ rock** (a rotting movie-palace pipe organ + rock band, made in Runway).
Drop the .ogg files into `engine/content/music` with these names; any that are missing fall back to `mm_level1`:
| file | where | style |
|---|---|---|
| `mm_title` | title screen | wheezing organ intro that kicks into the full band |
| `mm_level1` | Lobby, Auditorium | fast organ-lead run-and-gun rock |
| `mm_boiler` | Boiler Room | same melody over clanking metal percussion and steam |
| `mm_backstage` | Backstage (Ink Fiend) | band drops out, lone music box plays the melody slowly |
| `mm_booth` | Projection Booth | building tension, ticking projector rhythm, organ swells |
| `mm_boss1` | Uncle Inky | heavy organ-metal waltz in 3/4 |
| `mm_clear` | level clear | short triumphant organ fanfare |
| `mm_gameover` | game over | sad organ winding down like a dying machine |
| `mm_level2` | Pizza Party | party-organ polka rock, carnival calliope over the band |
| `mm_kitchen` | Kitchen, Delivery Dock | the same tune over sizzling, clattering pots and machinery |
| `mm_boss2` | Big Cheese | stomping organ-metal tarantella |

All eleven tracks were generated in Runway around one shared D-minor organ theme, with LOOPSTART tags so loops skip the intros.

## Gore and effects
Enemies bleed oil-blood on every hit and come apart when they die: the sprite is chopped into chunks, plus painted
body parts (porcelain shards, eyeballs on wires, leaking gloves, gears, springs, lens glass) that bounce and
leave stains. Blood and ink stay splattered on the floor and walls. The welding torch sets enemies on fire, rivets
burst, nails spark, casings fly, big hits freeze the action for a few frames, and Uncle Inky's slams send out
shockwaves and splinters. When he dies he blows apart in stages, then shatters, mask and all.
Test switches: `MM_WEAPON=0..3` (starting weapon), `MM_BOSS_HP=n` (boss starting health).

## Sound effects
32 custom effects generated in Runway (`engine/content/sfx`, loudness-matched WAVs): nail gun, rivet blast, welding
torch, plunger thwonk, hits, worker "oof" with a slide whistle, porcelain shatter, gear clatter, ink splats, gory
bursts, casing tinks, projector wind-up and beam, breaker clunk, stage lights, power failure, closet door, Ink Fiend
roar and warning stinger, tentacle slam, Uncle Inky's angry roar and dying wail, ghost-light checkpoint and organ UI blips.
