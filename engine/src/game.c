/* MIDNIGHT MATINEE - Level 1 "Opening Night"
 * Run-and-gun through Uncle Inky's Jamboree Palace. The GamePad is the security office:
 * camera feeds, PING (mark enemies) and LIGHTS (flood a zone, scare the Ink Fiend).
 */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gfx.h"
#include "audio.h"
#include "game.h"
#include "save.h"

#define TILE 48
#define LW 312          /* every show: 240 authored columns + three 24-column stretches opened up inside it */
#define OLD_LW 240      /* the width the level builders are written for */
#define STRETCH 24
#define LH 15
#define FLOOR_Y (13 * TILE)
#define DT (1.0f / 60.0f)
static int arena_tx = 212;   /* where the boss arena starts once the stretches are in */
#define ARENA_X (arena_tx * TILE)

static float frand(void) { return rand() / (float)RAND_MAX; }
static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static float signf(float v) { return v < 0 ? -1.0f : 1.0f; }

/* ==================================================================================================
   ESPAÑOL: every string on its way to the screen goes through TR(). Longest English phrases are matched
   first, at word boundaries, in one left-to-right pass, so built strings ("SHOW 3: PIZZA PARTY") translate too.
   ================================================================================================== */
static const char *const TR_TAB[][2] = {
    {"UNCLE INKY IS BACK, AND HIS SHOW NEVER ENDS. KEEP THE LIGHTS ON. SURVIVE THE MATINEE.", "EL T\x04""O INKY HA VUELTO Y SU FUNCI\x05""N NUNCA TERMINA. MANT\x03""N LA LUZ. SOBREVIVE A LA MATIN\x03""E."},
    {"BUT THE MOMENT THE LIGHTS CAME UP, THE OLD PROJECTOR STARTED ROLLING ALL BY ITSELF.", "PERO EN CUANTO VOLVI\x05"" LA LUZ, EL VIEJO PROYECTOR SE ENCENDI\x05"" SOLITO."},
    {"THE GAMEPAD RUNS THE SECURITY CAMERAS: TAP A FEED, PING ENEMIES, HIT THE LIGHTS.", "EL GAMEPAD CONTROLA LAS C\x02""MARAS: TOCA UNA, MARCA ENEMIGOS, ENCIENDE LAS LUCES."},
    {"JOE AND ROSIE TOOK THE NIGHT SHIFT: GET THE OLD PLACE'S POWER BACK ON.", "JOE Y ROSIE TOMARON EL TURNO DE NOCHE: DEVOLVERLE LA LUZ AL VIEJO TEATRO."},
    {"MIDNIGHT. THE GRAND MATINEE THEATRE HAS BEEN DARK FOR FIFTY YEARS...", "MEDIANOCHE. EL GRAN TEATRO MATIN\x03""E LLEVA CINCUENTA A\x01""OS A OSCURAS..."},
    {"A SECOND PLAYER CAN JUMP IN ANY TIME: PRESS + ON ANOTHER CONTROLLER.", "UN SEGUNDO JUGADOR PUEDE ENTRAR CUANDO QUIERA: PULSA + EN OTRO CONTROL."},
    {"PRESS [OK] - THEN THE SECOND SHOW (HARDER - THE TRUE ENDING AWAITS)", "PULSA [OK] - LUEGO LA SEGUNDA FUNCI\x05""N (M\x02""S DIF\x04""CIL - TE ESPERA EL FINAL VERDADERO)"},
    {"BEAT A SHOW IN UNDER SIX MINUTES WITHOUT A KNOCKOUT FOR AN S RANK.", "TERMINA UNA FUNCI\x05""N EN MENOS DE SEIS MINUTOS SIN CAER PARA UN RANGO S."},
    {"THE SD CARD IS LOCKED OR MISSING - PROGRESS WON'T BE SAVED", "LA TARJETA SD EST\x02"" BLOQUEADA O NO EST\x02"" - NO SE GUARDAR\x02"" EL PROGRESO"},
    {"IN THE DARK, EVERYTHING GETS FASTER. KEEP THE POWER UP!", "EN LA OSCURIDAD TODO VA M\x02""S R\x02""PIDO. \x08""MANT\x03""N LA ENERG\x04""A!"},
    {"WHICH ONE IS THE REAL MADAME? (THE FAKES ARE BACKWARDS)", "\x07""CU\x02""L ES LA VERDADERA MADAME? (LAS FALSAS EST\x02""N AL REV\x03""S)"},
    {"HOLD R TO PLANT YOUR FEET AND AIM IN EIGHT DIRECTIONS.", "MANT\x03""N R PARA QUEDARTE QUIETO Y APUNTAR EN OCHO DIRECCIONES."},
    {"THE NIGHT CREW CLOCKS OUT. THE SHOW IS OVER FOR GOOD.", "EL TURNO DE NOCHE TERMINA. LA FUNCI\x05""N SE ACAB\x05"" PARA SIEMPRE."},
    {"CONNECT A WII REMOTE OR PRO CONTROLLER FOR THE RUNNER", "CONECTA UN WIIMOTE O UN PRO CONTROLLER PARA EL CORREDOR"},
    {"THREE FILM REELS ARE HIDDEN IN EVERY SHOW. LOOK UP!", "HAY TRES ROLLOS DE PEL\x04""CULA ESCONDIDOS EN CADA FUNCI\x05""N. \x08""MIRA ARRIBA!"},
    {"FLIP A BREAKER (PRESS UP) TO KEEP THE LIGHTS ON.", "SUBE UN INTERRUPTOR (ARRIBA) PARA MANTENER LA LUZ."},
    {"CO-OP: RUNNER USES A WII REMOTE / PRO CONTROLLER", "COOPERATIVO: EL CORREDOR USA UN WIIMOTE / PRO CONTROLLER"},
    {"THE SECOND SHOW: EVERYTHING IS FASTER AND MEANER", "LA SEGUNDA FUNCI\x05""N: TODO ES M\x02""S R\x02""PIDO Y M\x02""S MALO"},
    {"SOLO: THIS GAMEPAD RUNS AND WATCHES THE CAMERAS", "SOLO: ESTE GAMEPAD CORRE Y VIGILA LAS C\x02""MARAS"},
    {"LOST: ONE COLLECTOR. NOBODY CAME TO CLAIM HIM.", "PERDIDO: UN COLECCIONISTA. NADIE VINO A RECLAMARLO."},
    {"PRESS + TO PAUSE. THE SHOW WILL WAIT FOR YOU.", "PULSA + PARA PAUSAR. LA FUNCI\x05""N TE ESPERA."},
    {"FIND MORE FILM REELS TO UNLOCK THIS OUTFIT", "ENCUENTRA M\x02""S ROLLOS PARA DESBLOQUEAR ESTE TRAJE"},
    {"BREAK CRATES: SOME HIDE A FRESH HARD HAT.", "ROMPE CAJAS: ALGUNAS ESCONDEN UN CASCO NUEVO."},
    {"THE LIGHTS COME UP. THE INK DRAINS AWAY.", "SE ENCIENDEN LAS LUCES. LA TINTA SE ESCURRE."},
    {"FIND THIS SHOW'S FILM REEL TO DEVELOP IT", "ENCUENTRA EL ROLLO DE ESTA FUNCI\x05""N PARA REVELARLO"},
    {"THE BOSSES TAKE A BOW... AT YOUR EXPENSE", "LOS JEFES SALUDAN AL P\x06""BLICO... A TU COSTA"},
    {"UNCLE INKY KEEPS HIS THEATRE... FOR NOW", "EL T\x04""O INKY SE QUEDA CON SU TEATRO... POR AHORA"},
    {"FINISH THE BOSS RUSH IN UNDER 8 MINUTES", "TERMINA EL DESFILE DE JEFES EN MENOS DE 8 MINUTOS"},
    {"THE SHOW IS PAUSED - PRESS + TO RESUME", "FUNCI\x05""N EN PAUSA - PULSA + PARA SEGUIR"},
    {"SAVE DATA ERASED. A FRESH SHOW AWAITS.", "DATOS BORRADOS. UNA NUEVA FUNCI\x05""N TE ESPERA."},
    {"CAMERAS OFFLINE UNTIL THE SHOW STARTS", "C\x02""MARAS APAGADAS HASTA QUE EMPIECE LA FUNCI\x05""N"},
    {"A FAKE! SEVEN YEARS BAD LUCK FOR HER!", "\x08""UNA FALSA! \x08""SIETE A\x01""OS DE MALA SUERTE PARA ELLA!"},
    {"UNCLE INKY'S BATON HAS BEEN RETIRED.", "LA BATUTA DEL T\x04""O INKY SE HA JUBILADO."},
    {"THE KITCHEN IS CLOSED. PERMANENTLY.", "LA COCINA EST\x02"" CERRADA. PARA SIEMPRE."},
    {"THE INK DRAINS AWAY... OR DOES IT?", "LA TINTA SE ESCURRE... \x07""O NO?"},
    {"IT'S HERE! HIDE IN A PROP CLOSET!", "\x08""YA LLEG\x05""! \x08""ESC\x05""NDETE EN UN ARMARIO!"},
    {"HIS HEART IS SHOWING! AIM FOR IT!", "\x08""SE LE VE EL CORAZ\x05""N! \x08""APUNTA AH\x04""!"},
    {"KEEP PLAYING TO UNLOCK THIS TRACK", "SIGUE JUGANDO PARA DESBLOQUEAR ESTA CANCI\x05""N"},
    {"BEAT SHOW 8 TO OPEN THE BOSS RUSH", "TERMINA LA FUNCI\x05""N 8 PARA ABRIR EL DESFILE DE JEFES"},
    {"THE CURTAIN FALLS ON UNCLE INKY!", "\x08""CAE EL TEL\x05""N PARA EL T\x04""O INKY!"},
    {"HIS NAME IS NO LONGER IN LIGHTS.", "SU NOMBRE YA NO BRILLA EN LA MARQUESINA."},
    {"HE'S CALLING DOWN THE LIGHTNING!", "\x08""EST\x02"" LLAMANDO A LOS RAYOS!"},
    {"HIS UNDERSTUDIES TAKE THE STAGE!", "\x08""SUS SUPLENTES SALEN AL ESCENARIO!"},
    {"SOMETHING IS MOVING BACKSTAGE...", "ALGO SE MUEVE TRAS BAMBALINAS..."},
    {"UNCLE INKY IS FINALLY AT REST...", "EL T\x04""O INKY POR FIN DESCANSA..."},
    {"EVERYTHING THE THEATRE EVER LOST", "TODO LO QUE EL TEATRO PERDI\x05"""},
    {"PRESS AGAIN TO ERASE EVERYTHING!", "\x08""PULSA OTRA VEZ PARA BORRAR TODO!"},
    {"LIGHTNING ROD! HE TOOK THE HIT!", "\x08""PARARRAYOS! \x08""\x03""L SE LLEV\x05"" EL GOLPE!"},
    {"THE INK IS RISING! GET UP HIGH!", "\x08""SUBE LA TINTA! \x08""VE A LO ALTO!"},
    {"ART, ANIMATION, MUSIC AND SOUND", "ARTE, ANIMACI\x05""N, M\x06""SICA Y SONIDO"},
    {"BEAT A SHOW WITHOUT GETTING HIT", "TERMINA UNA FUNCI\x05""N SIN RECIBIR GOLPES"},
    {"BEAT A BOSS WITHOUT GETTING HIT", "VENCE A UN JEFE SIN RECIBIR GOLPES"},
    {"SEE YOU AT THE NEXT MATINEE...", "NOS VEMOS EN LA PR\x05""XIMA MATIN\x03""E..."},
    {"PRESS [OK] TO ROLL THE CREDITS", "PULSA [OK] PARA VER LOS CR\x03""DITOS"},
    {"YOUR NAME IN LIGHTS... FOREVER", "TU NOMBRE EN LUCES... PARA SIEMPRE"},
    {"BEAT A SHOW IN UNDER 4 MINUTES", "TERMINA UNA FUNCI\x05""N EN MENOS DE 4 MINUTOS"},
    {"BEAT A SHOW ON MIDNIGHT (HARD)", "TERMINA UNA FUNCI\x05""N EN MEDIANOCHE (DIF\x04""CIL)"},
    {"LIGHTS OUT FOR THE HEADLINER!", "\x08""SE LE APAGAN LAS LUCES AL ESTELAR!"},
    {"IT PRINTS NOTHING BUT TROUBLE", "SOLO IMPRIME PROBLEMAS"},
    {"THE BANDLEADER WHO NEVER LEFT", "EL DIRECTOR DE BANDA QUE NUNCA SE FUE"},
    {"THE CHEF WHO ATE THE BIRTHDAY", "EL CHEF QUE SE COMI\x05"" EL CUMPLEA\x01""OS"},
    {"ZL / ZR OR TAP: SWITCH CAMERA", "ZL / ZR O TOCA: CAMBIAR C\x02""MARA"},
    {"BEAT A SHOW IN SECURITY CO-OP", "TERMINA UNA FUNCI\x05""N EN COOP DE SEGURIDAD"},
    {"HE'S TRYING TO BAG YOU! RUN!", "\x08""TE QUIERE METER EN SU COSTAL! \x08""CORRE!"},
    {"SHE SUMMONS YOUR REFLECTION!", "\x08""INVOCA TU REFLEJO!"},
    {"REACH THIS SHOW TO UNLOCK IT", "LLEGA A ESTA FUNCI\x05""N PARA DESBLOQUEARLA"},
    {"SPLASH! BACK ON SOLID GROUND", "\x08""SPLASH! DE VUELTA EN TIERRA FIRME"},
    {"UP ON THE ROOF, IN THE STORM", "EN LA AZOTEA, BAJO LA TORMENTA"},
    {"SHE CUTS EVERY PICTURE SHORT", "CORTA TODAS LAS PEL\x04""CULAS"},
    {"UNCLE INKY'S JAMBOREE PALACE", "EL PALACIO DE FIESTA DEL T\x04""O INKY"},
    {"BEAT A SHOW WITH TWO PLAYERS", "TERMINA UNA FUNCI\x05""N CON DOS JUGADORES"},
    {"GHOST LIGHT LIT: CHECKPOINT", "LUZ FANTASMA ENCENDIDA: PUNTO DE CONTROL"},
    {"NO ANIMATRONICS WERE HARMED", "NING\x06""N ANIMATR\x05""NICO SALI\x05"" HERIDO"},
    {"UNDEVELOPED: FIND FILM REEL", "SIN REVELAR: ENCUENTRA EL ROLLO"},
    {"EARN S RANKS ON ALL 8 SHOWS", "CONSIGUE RANGO S EN LAS 8 FUNCIONES"},
    {"YOU ALREADY HAVE THIS REEL", "YA TIENES ESTE ROLLO"},
    {"HIDING... (UP TO COME OUT)", "ESCONDIDO... (ARRIBA PARA SALIR)"},
    {"LIGHTS SCARE THE INK FIEND", "LAS LUCES ASUSTAN AL DEMONIO DE TINTA"},
    {"POWER OUT! FIND A BREAKER", "\x08""SIN LUZ! BUSCA UN INTERRUPTOR"},
    {"THE LIGHTS DROVE IT BACK!", "\x08""LAS LUCES LO HICIERON RETROCEDER!"},
    {"THE JAMBOREE PIZZA PARLOR", "LA PIZZER\x04""A JAMBOREE"},
    {"THE STARS' DRESSING ROOMS", "LOS CAMERINOS DE LAS ESTRELLAS"},
    {"FIND YOUR FIRST FILM REEL", "ENCUENTRA TU PRIMER ROLLO"},
    {"THERE GOES THE HARD HAT!", "\x08""ADI\x05""S AL CASCO!"},
    {"DOWN TO YOUR UNDERSHIRT!", "\x08""TE QUEDASTE EN CAMISETA!"},
    {"THE STAR'S DRESSING ROOM", "EL CAMERINO DE LA ESTRELLA"},
    {"PRESS [X] AGAIN TO ERASE", "PULSA [X] OTRA VEZ PARA BORRAR"},
    {"THE FAIREST OF THEM ALL", "LA M\x02""S BELLA DE TODAS"},
    {"STARRING THE NIGHT CREW", "PROTAGONIZADA POR EL TURNO DE NOCHE"},
    {"UNCLE INKY TAKES A BOW!", "\x08""EL T\x04""O INKY HACE UNA REVERENCIA!"},
    {"ELECTRICIAN OF THE YEAR", "ELECTRICISTA DEL A\x01""O"},
    {"THE LOST CARTOON VAULT", "LA B\x05""VEDA DE CARICATURAS PERDIDAS"},
    {"GUST OF WIND! HOLD ON!", "\x08""R\x02""FAGA DE VIENTO! \x08""AG\x02""RRATE!"},
    {"HE'S SPELLING TROUBLE!", "\x08""EST\x02"" DELETREANDO PROBLEMAS!"},
    {"BREAKER ON: +40% POWER", "INTERRUPTOR: +40% DE ENERG\x04""A"},
    {"HOLD [OK] FAST FORWARD", "MANT\x03""N [OK] ADELANTAR"},
    {"FIND ALL 24 FILM REELS", "ENCUENTRA LOS 24 ROLLOS"},
    {"WHERE THE INK IS MADE", "DONDE SE HACE LA TINTA"},
    {"HE WANTS IT ALL BACK!", "\x08""LO QUIERE TODO DE VUELTA!"},
    {"THE SPLICER GETS CUT!", "\x08""A LA EDITORA LA CORTARON!"},
    {"P2: PRESS [+] TO JOIN", "J2: PULSA [+] PARA ENTRAR"},
    {"RESTART THE BOSS RUSH", "REINICIAR EL DESFILE DE JEFES"},
    {"HIDE IN A PROP CLOSET", "ESC\x05""NDETE EN UN ARMARIO"},
    {"DOWN TO YOUR BOXERS!", "\x08""TE QUEDASTE EN CALZONES!"},
    {"IT'S COMING... HIDE!", "YA VIENE... \x08""ESC\x05""NDETE!"},
    {"THE MARQUEE  (TITLE)", "LA MARQUESINA  (T\x04""TULO)"},
    {"SEVEN YEARS BAD LUCK", "SIETE A\x01""OS DE MALA SUERTE"},
    {"THAT'S THE REAL ONE!", "\x08""ESA ES LA VERDADERA!"},
    {"BIG CHEESE IS TOAST!", "\x08""EL GRAN QUESO EST\x02"" FRITO!"},
    {"THE FINAL CURTAIN...", "EL TEL\x05""N FINAL..."},
    {"THE WHOLE COLLECTION", "LA COLECCI\x05""N COMPLETA"},
    {"FINISH THE BOSS RUSH", "TERMINA EL DESFILE DE JEFES"},
    {"AND TONIGHT'S SHOWS", "Y LAS FUNCIONES DE ESTA NOCHE"},
    {"CHASE LIGHTS! JUMP!", "\x08""LUCES! \x08""SALTA!"},
    {"CLAIM TICKET: VOID!", "\x08""BOLETO ANULADO!"},
    {"PLAYER 2 CLOCKS IN!", "\x08""ENTRA EL JUGADOR 2!"},
    {"TRY THIS SHOW AGAIN", "REINTENTAR ESTA FUNCI\x05""N"},
    {"BOSS RUSH COMPLETE!", "\x08""DESFILE DE JEFES COMPLETADO!"},
    {"THE SHOW MUST GO ON", "LA FUNCI\x05""N DEBE CONTINUAR"},
    {"FIRST NIGHT JITTERS", "NERVIOS DE ESTRENO"},
    {"SEE THE TRUE ENDING", "VE EL FINAL VERDADERO"},
    {"EYES ON THE CAMERAS", "OJOS EN LAS C\x02""MARAS"},
    {"DRESSED FOR SUCCESS", "VESTIDO PARA TRIUNFAR"},
    {"CAM 4: HIT LIGHTS!", "C\x02""M 4: \x08""LUCES!"},
    {"SHE'S CRACKING UP!", "\x08""SE EST\x02"" AGRIETANDO!"},
    {"COVERALLS BACK ON!", "\x08""OTRA VEZ CON OVEROL!"},
    {"PRESS [+] TO START", "PULSA [+] PARA EMPEZAR"},
    {"CHOOSE A SAVE SLOT", "ELIGE UNA RANURA"},
    {"QUIT THE BOSS RUSH", "SALIR DEL DESFILE DE JEFES"},
    {"A JUMPS / B SHOOTS", "A SALTA / B DISPARA"},
    {"B JUMPS / A SHOOTS", "B SALTA / A DISPARA"},
    {"THE SPLICING TABLE", "LA MESA DE EDICI\x05""N"},
    {"FIND 12 FILM REELS", "ENCUENTRA 12 ROLLOS"},
    {"SHE'S UNSPOOLING!", "\x08""SE EST\x02"" DESENROLLANDO!"},
    {"IT'S OVERHEATING!", "\x08""SE EST\x02"" SOBRECALENTANDO!"},
    {"HE'S OVERLOADING!", "\x08""SE EST\x02"" SOBRECARGANDO!"},
    {"STOP THE PRESSES!", "\x08""PAREN LAS PRENSAS!"},
    {"OVERALLS BACK ON!", "\x08""OTRA VEZ CON PETO!"},
    {"PRESS [+] TO PLAY", "PULSA [+] PARA JUGAR"},
    {"RESTART THIS SHOW", "REINICIAR ESTA FUNCI\x05""N"},
    {"THE CURTAIN FALLS", "CAE EL TEL\x05""N"},
    {"WEAR A NEW OUTFIT", "PONTE UN TRAJE NUEVO"},
    {"FILM REEL FOUND!", "\x08""ROLLO ENCONTRADO!"},
    {"THE SHOW IS OVER", "SE ACAB\x05"" LA FUNCI\x05""N"},
    {"FILM REELS FOUND", "ROLLOS ENCONTRADOS"},
    {"STAGEHAND BADGES", "INSIGNIAS DE TRAMOYA"},
    {"PLUNGER LAUNCHER", "LANZADESTAPADORES"},
    {"PROJECTION BOOTH", "CABINA DE PROYECCI\x05""N"},
    {"STAR OF THE SHOW", "ESTRELLA DEL SHOW"},
    {"STANDING OVATION", "OVACI\x05""N DE PIE"},
    {"FLIP 25 BREAKERS", "SUBE 25 INTERRUPTORES"},
    {"MIND THE GLASS!", "\x08""CUIDADO CON EL VIDRIO!"},
    {"WATCH THE INTRO", "VER LA INTRODUCCI\x05""N"},
    {"ERASE SAVE DATA", "BORRAR DATOS"},
    {"SECURITY OFFICE", "OFICINA DE SEGURIDAD"},
    {"MIDNIGHT (HARD)", "MEDIANOCHE (DIF\x04""CIL)"},
    {"UP AT A BREAKER", "ARRIBA EN UN INTERRUPTOR"},
    {"HALL OF MIRRORS", "SALA DE ESPEJOS"},
    {"ROOFTOP MARQUEE", "MARQUESINA EN LA AZOTEA"},
    {"FINDERS KEEPERS", "EL QUE LO ENCUENTRA SE LO QUEDA"},
    {"THE GRAND STAGE", "EL GRAN ESCENARIO"},
    {"ENCORE! ENCORE!", "\x08""OTRA! \x08""OTRA!"},
    {"THAT'S A WRAP!", "\x08""CORTE Y QUEDA!"},
    {"START THE SHOW", "EMPEZAR LA FUNCI\x05""N"},
    {"NEW BEST RANK!", "\x08""NUEVO MEJOR RANGO!"},
    {"SECURITY CO-OP", "COOP DE SEGURIDAD"},
    {"CO-OP OPERATOR", "OPERADOR COOP"},
    {"MATINEE (EASY)", "MATIN\x03""E (F\x02""CIL)"},
    {"UP AT A CLOSET", "ARRIBA EN UN ARMARIO"},
    {"D-PAD OR STICK", "CRUCETA O PALANCA"},
    {"THE FINAL SHOW", "LA \x06""LTIMA FUNCI\x05""N"},
    {"BARNYARD SHORT", "CORTO DE GRANJA"},
    {"BOILER FURNACE", "HORNO"},
    {"THE PRESS ROOM", "LA SALA DE PRENSAS"},
    {"THE CLAIM DESK", "EL MOSTRADOR DE RECLAMOS"},
    {"LIGHTNING RODS", "PARARRAYOS"},
    {"EARN AN S RANK", "CONSIGUE UN RANGO S"},
    {"HE'S STEAMED!", "\x08""EST\x02"" QUE ECHA HUMO!"},
    {"AN EMPTY SEAT", "UN ASIENTO VAC\x04""O"},
    {"QUIT TO TITLE", "SALIR AL T\x04""TULO"},
    {"SHOW CLEARED!", "\x08""FUNCI\x05""N SUPERADA!"},
    {"SOUND EFFECTS", "EFECTOS DE SONIDO"},
    {"WELDING TORCH", "SOPLETE"},
    {"GOLDEN TICKET", "BOLETO DORADO"},
    {"SILVER SCREEN", "PANTALLA DE PLATA"},
    {"OPENING NIGHT", "NOCHE DE ESTRENO"},
    {"CARTOON REELS", "ROLLOS DE CARICATURA"},
    {"THE INK WORKS", "LA F\x02""BRICA DE TINTA"},
    {"THE BIG PRESS", "LA GRAN PRENSA"},
    {"THE COLLECTOR", "EL COLECCIONISTA"},
    {"THE HEADLINER", "EL ESTELAR"},
    {"PIZZA COUNTER", "MOSTRADOR"},
    {"DELIVERY DOCK", "AND\x03""N DE REPARTO"},
    {"THE RAT'S DEN", "LA GUARIDA DE LA RATA"},
    {"LOST PROPERTY", "COSAS PERDIDAS"},
    {"DOLL HOSPITAL", "HOSPITAL DE MU\x01""ECAS"},
    {"THE NEON SIGN", "EL LETRERO DE NE\x05""N"},
    {"BURNING LOBBY", "VEST\x04""BULO EN LLAMAS"},
    {"ORCHESTRA PIT", "FOSO DE ORQUESTA"},
    {"HALFWAY THERE", "A MEDIO CAMINO"},
    {"THE FINAL BOW", "LA \x06""LTIMA REVERENCIA"},
    {"TRUE BELIEVER", "CREYENTE DE VERDAD"},
    {"HARD HAT HERO", "H\x03""ROE DEL CASCO"},
    {"HIDE AND SEEK", "LAS ESCONDIDAS"},
    {"IN THIS SHOW", "EN ESTA FUNCI\x05""N"},
    {"MUSIC VOLUME", "VOLUMEN DE M\x06""SICA"},
    {"SCREEN SHAKE", "TEMBLOR DE PANTALLA"},
    {"BADGE EARNED", "INSIGNIA GANADA"},
    {"FAST FORWARD", "ADELANTAR"},
    {"INTERMISSION", "INTERMEDIO"},
    {"RIVET SPREAD", "REMACHES"},
    {"LOST & FOUND", "OBJETOS PERDIDOS"},
    {"INKY UNBOUND", "INKY DESATADO"},
    {"CUTTING ROOM", "SALA DE EDICI\x05""N"},
    {"LUGGAGE HALL", "SALA DE EQUIPAJE"},
    {"WATER TOWERS", "TANQUES DE AGUA"},
    {"THE FLY LOFT", "EL TELAR"},
    {"CURTAIN CALL", "SALUDO FINAL"},
    {"QUICK CHANGE", "CAMBIO R\x02""PIDO"},
    {"MIDNIGHT RUN", "CARRERA DE MEDIANOCHE"},
    {"BUDDY SYSTEM", "TRABAJO EN EQUIPO"},
    {"NEW RECORD!", "\x08""NUEVO R\x03""CORD!"},
    {"TRUE ENDING", "FINAL VERDADERO"},
    {"NOW SHOWING", "EN CARTELERA"},
    {"NOW PLAYING", "SONANDO"},
    {"SHOW SELECT", "ELEGIR FUNCI\x05""N"},
    {"FIEND ALERT", "ALERTA DE DEMONIO"},
    {"SLOT ERASED", "RANURA BORRADA"},
    {"PROGRAMMING", "PROGRAMACI\x05""N"},
    {"NIGHT SHIFT", "TURNO DE NOCHE"},
    {"INK STAINED", "MANCHADO DE TINTA"},
    {"PIZZA PARTY", "FIESTA DE PIZZA"},
    {"THE SPLICER", "LA EDITORA"},
    {"BOILER ROOM", "SALA DE CALDERAS"},
    {"DINING HALL", "COMEDOR"},
    {"SPOOK SHORT", "CORTO DE SUSTOS"},
    {"LOADING BAY", "ZONA DE CARGA"},
    {"PRESS FLOOR", "PISO DE PRENSAS"},
    {"FIRE ESCAPE", "ESCALERA DE INCENDIOS"},
    {"THE ROOFTOP", "LA AZOTEA"},
    {"THE MARQUEE", "LA MARQUESINA"},
    {"THE BALCONY", "EL BALC\x05""N"},
    {"BEAT SHOW 1", "TERMINA LA FUNCI\x05""N 1"},
    {"BEAT SHOW 4", "TERMINA LA FUNCI\x05""N 4"},
    {"BEAT SHOW 8", "TERMINA LA FUNCI\x05""N 8"},
    {"UNTOUCHABLE", "INTOCABLE"},
    {"FULL GEAR!", "\x08""EQUIPO COMPLETO!"},
    {"SAVE SLOTS", "RANURAS"},
    {"FILM REELS", "ROLLOS"},
    {"SHIFT OVER", "FIN DEL TURNO"},
    {"PLAY STYLE", "MODO DE JUEGO"},
    {"DIFFICULTY", "DIFICULTAD"},
    {"BEST SCORE", "MEJOR PUNTAJE"},
    {"OR", "O"},
    {"SOMETHING'S MISSING BACKSTAGE", "FALTA ALGO TRAS BAMBALINAS"},
    {"SOME OF THE GAME'S FILES COULDN'T BE FOUND.", "NO SE ENCONTRARON ALGUNOS ARCHIVOS DEL JUEGO."},
    {"PLEASE REINSTALL MIDNIGHT MATINEE FROM THE .WUHB FILE.", "VUELVE A INSTALAR MIDNIGHT MATINEE DESDE EL ARCHIVO .WUHB."},
    {"TO QUIT", "PARA SALIR"},
    {"CONTROLLER DISCONNECTED", "CONTROL DESCONECTADO"},
    {"A CONTROLLER LOST ITS CONNECTION.", "UN CONTROL PERDI\x05"" LA CONEXI\x05""N."},
    {"CHECK ITS BATTERY OR RECONNECT IT. THE SHOW WILL WAIT.", "REVISA LA BATER\x04""A O VUELVE A CONECTARLO. LA FUNCI\x05""N ESPERAR\x02""."},
    {"PRESS ANY BUTTON TO CONTINUE", "PULSA CUALQUIER BOT\x05""N PARA CONTINUAR"},
    {"LOADING...", "CARGANDO..."},
    {"UNDERSHIRT", "CAMISETA"},
    {"UNCLE INKY", "T\x04""O INKY"},
    {"BIG CHEESE", "GRAN QUESO"},
    {"AUDITORIUM", "AUDITORIO"},
    {"MAIN STAGE", "ESCENARIO PRINCIPAL"},
    {"PARTY ROOM", "SAL\x05""N DE FIESTAS"},
    {"TITLE CARD", "TARJETA DE T\x04""TULO"},
    {"FILM VAULT", "B\x05""VEDA DE PEL\x04""CULAS"},
    {"PIPE WORKS", "TUBER\x04""AS"},
    {"COAT CHECK", "GUARDARROPA"},
    {"STAGE DOOR", "PUERTA DE ARTISTAS"},
    {"MAKEUP ROW", "FILA DE MAQUILLAJE"},
    {"THE VANITY", "EL TOCADOR"},
    {"INK RAIN!", "\x08""LLUVIA DE TINTA!"},
    {"BOSS RUSH", "DESFILE DE JEFES"},
    {"SAVE SLOT", "RANURA"},
    {"FILM REEL", "ROLLO"},
    {"LIGHTS ON", "LUZ"},
    {"SWAP GUNS", "CAMBIAR ARMA"},
    {"BEST RANK", "MEJOR RANGO"},
    {"BEST TIME", "MEJOR TIEMPO"},
    {"CONTINUE:", "CONTINUAR:"},
    {"(WEARING)", "(PUESTO)"},
    {"A GAME BY", "UN JUEGO DE"},
    {"KNOCKOUTS", "CA\x04""DAS"},
    {"FULL GEAR", "EQUIPO COMPLETO"},
    {"COVERALLS", "OVEROL"},
    {"BACKSTAGE", "TRAS BAMBALINAS"},
    {"TOY CHEST", "BA\x06""L DE JUGUETES"},
    {"REEL FIND", "PRIMER HALLAZGO"},
    {"FILM BUFF", "CIN\x03""FILO"},
    {"RUSH HOUR", "HORA PICO"},
    {"NO POWER", "SIN LUZ"},
    {"LANGUAGE", "IDIOMA"},
    {"BIG JUMP", "SALTO ALTO"},
    {"OR STICK", "O PALANCA"},
    {"CLEARED!", "\x08""SUPERADA!"},
    {"(LOCKED)", "(BLOQUEADO)"},
    {"2ND SHOW", "2A FUNCI\x05""N"},
    {"NEW GAME", "NUEVA PARTIDA"},
    {"WARDROBE", "VESTUARIO"},
    {"TO START", "PARA EMPEZAR"},
    {"OVERALLS", "PETO"},
    {"NAIL GUN", "CLAVADORA"},
    {"INK VATS", "TINAS DE TINTA"},
    {"ENGLISH", "ENGLISH"},
    {"ESPANOL", "ESPA\x01""OL"},
    {"RUNNER:", "CORREDOR:"},
    {"CLEARED", "SUPERADA"},
    {"CREDITS", "CR\x03""DITOS"},
    {"OPTIONS", "OPCIONES"},
    {"GALLERY", "GALER\x04""A"},
    {"JUKEBOX", "ROCOLA"},
    {"BUTTONS", "BOTONES"},
    {"TO JOIN", "PARA ENTRAR"},
    {"CLASSIC", "CL\x02""SICO"},
    {"KITCHEN", "COCINA"},
    {"ENGINE", "MOTOR"},
    {"EXTRAS", "EXTRAS"},
    {"RUMBLE", "VIBRACI\x05""N"},
    {"RESUME", "CONTINUAR"},
    {"SELECT", "ACEPTAR"},
    {"CHOOSE", "ELEGIR"},
    {"CHANGE", "CAMBIAR"},
    {"BROWSE", "VER"},
    {"LOCKED", "BLOQUEADO"},
    {"BUTTON", "BOT\x05""N"},
    {"NORMAL", "NORMAL"},
    {"BOXERS", "CALZONES"},
    {"ENCORE", "OTRA VEZ"},
    {"BEST!", "\x08""MEJOR!"},
    {"NEXT:", "SIGUE:"},
    {"DOWN!", "\x08""DERROTADO!"},
    {"ERASE", "BORRAR"},
    {"PAUSE", "PAUSA"},
    {"SCORE", "PUNTOS"},
    {"POWER", "ENERG\x04""A"},
    {"SHOOT", "DISPARAR"},
    {"PRESS", "PULSA"},
    {"REELS", "ROLLOS"},
    {"TOOLS", "\x06""TILES"},
    {"PROPS", "UTILER\x04""A"},
    {"LOBBY", "VEST\x04""BULO"},
    {"VIEW", "VER"},
    {"WEAR", "PONER"},
    {"OPEN", "ABRIR"},
    {"BACK", "ATR\x02""S"},
    {"PLAY", "JUGAR"},
    {"NEXT", "SIGUE"},
    {"SKIP", "SALTAR"},
    {"RANK", "RANGO"},
    {"TIME", "TIEMPO"},
    {"MOVE", "MOVER"},
    {"JUMP", "SALTAR"},
    {"HOLD", "MANT\x03""N"},
    {"HIDE", "ESCONDERSE"},
    {"SOLO", "SOLO"},
    {"BOSS", "JEFE"},
    {"SLOT", "RANURA"},
    {"SHOW", "FUNCI\x05""N"},
    {"DEMO", "DEMO"},
    {"GEAR", "EQUIPO"},
    {"AIM", "APUNTAR"},
    {"OFF", "NO"},
    {"ON", "S\x04"""},
    {"OF", "DE"},
};
#define NTR (int)(sizeof TR_TAB / sizeof *TR_TAB)
static int is_word(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '\''; }
static const char *TR(const char *s) {
    if (!save.language || !s || !*s) return s;
    static struct { unsigned h; char in[192]; char out[288]; } cache[160];
    unsigned h = 2166136261u; for (const char *p = s; *p; p++) h = (h ^ (unsigned char)*p) * 16777619u;
    int slot = (int)(h % 160);
    if (cache[slot].h == h && !strcmp(cache[slot].in, s)) return cache[slot].out;
    if (strlen(s) >= sizeof cache[0].in) return s;
    char out[288]; int n = 0; const char *p = s;
    while (*p && n < (int)sizeof out - 1) {
        int matched = 0;
        if (p == s || !is_word(p[-1]) || !is_word(*p)) {
            for (int i = 0; i < NTR; i++) { const char *e = TR_TAB[i][0]; size_t L = strlen(e);
                if (e[0] != *p || strncmp(p, e, L)) continue;
                if (is_word(e[L - 1]) && is_word(p[L])) continue;   /* "SHOW" must not eat the start of "SHOWS" */
                for (const char *t = TR_TAB[i][1]; *t && n < (int)sizeof out - 1; t++) out[n++] = *t;
                p += L; matched = 1; break; }
        }
        if (!matched) out[n++] = *p++;
    }
    out[n] = 0;
    cache[slot].h = h; snprintf(cache[slot].in, sizeof cache[slot].in, "%s", s); snprintf(cache[slot].out, sizeof cache[slot].out, "%s", out);
    return cache[slot].out;
}
/* every piece of text on its way to a screen goes through TR() (a no-op in English) */
#define draw_text_col(f, s, x, y, sc, c, r, g, b, a) (draw_text_col)(f, TR(s), x, y, sc, c, r, g, b, a)
#define text_width(f, s, sc) (text_width)(f, TR(s), sc)
static float fit(Font *f, const char *s, float scale, float maxw) { float w = text_width(f, s, scale); return w > maxw && w > 0 ? scale * maxw / w : scale; }

/* ------------------------------------------------------------ assets */
#define HERO_FRAMES 8
#define BG7_K 1.12f   /* painted floor height of the new show backgrounds */
#define BG8_K 1.12f
#define NCHARS 2   /* 0 Joe, 1 Rosie */
typedef struct { Tex hero[NCHARS][3][HERO_FRAMES]; Tex usher[5], dog[5], blob[4], inky[3], gore[12], goat[5], meat[5], slice[5], rat[5], cleaver, gore2[12], bg2, bomb[5], ghost[5], glove[5], splicer[3], bg3, imp[5], worker[5], eel[5], press[3], bg4, suitcase[5], soldier[5], umbrella[5], collector[3], bg5, unbound[3], bg6, bg7, bg8, mannequin[5], puff[5], crow[5], bulb[5], vanity[3], headliner[3]; Tex bg, disc, vignette, mask, grain[4]; Font font, small; } Assets;
static Assets A;
static Font padFont, padSmall; static Tex padDisc;

/* ------------------------------------------------------------ level */
static char map[LH][LW + 1];
enum { E_USHER, E_DOG, E_BLOB, E_MINIBLOB, E_GOAT, E_MEATBALL, E_SLICE, E_BOMB, E_GHOST, E_GLOVE, E_IMP, E_WORKER, E_EEL, E_SUITCASE, E_SOLDIER, E_UMBRELLA, E_SHADOW, E_MANNEQUIN, E_PUFF, E_MIRROR, E_CROW, E_BULB, E_CRATE, E_HATCRATE };
typedef struct { int type, alive, hp, face, state, flash, onground; float x, y, vx, vy, t, timer, marked, stun, plat0, plat1, burn, btick, land, mx; } Enemy;
#define MAXE 128
static Enemy en[MAXE]; static int nen;
typedef struct { int tx, used; } Breaker;
static Breaker breakers[8]; static int nbreak;
typedef struct { int tx; float t; } Drip;
static Drip drips[8]; static int ndrip;
static int checkpoints[12], ncheck, cur_check;

/* ---- the shows (levels) ---- */
typedef struct {
    const char *title, *subtitle, *boss_name, *boss_line, *boss_music;
    const char *zone[6]; int x0[7]; const char *music[6];
    int boss_hp; const char *kill_msg, *angry_msg;
} LevelDef;
enum { L_LOBBY, L_PIZZA, L_REELS, L_INK, L_LOST, L_MIRROR, L_ROOF, L_ENCORE, NUM_LEVELS };
static const LevelDef LEVELS[] = {
    {"OPENING NIGHT", "UNCLE INKY'S JAMBOREE PALACE", "UNCLE INKY", "THE BANDLEADER WHO NEVER LEFT", "mm_boss1",
     {"LOBBY", "AUDITORIUM", "BOILER ROOM", "BACKSTAGE", "PROJECTION BOOTH", "MAIN STAGE"}, {0, 40, 90, 130, 175, 210, LW},
     {"mm_level1", "mm_level1", "mm_boiler", "mm_backstage", "mm_booth", "mm_level1"}, 110, "UNCLE INKY TAKES A BOW!", "HE'S STEAMED!"},
    {"PIZZA PARTY", "THE JAMBOREE PIZZA PARLOR", "BIG CHEESE", "THE CHEF WHO ATE THE BIRTHDAY", "mm_boss2",
     {"PARTY ROOM", "DINING HALL", "PIZZA COUNTER", "KITCHEN", "DELIVERY DOCK", "THE RAT'S DEN"}, {0, 40, 85, 125, 170, 210, LW},
     {"mm_level2", "mm_level2", "mm_level2", "mm_kitchen", "mm_kitchen", "mm_level2"}, 140, "BIG CHEESE IS TOAST!", "HE'S STEAMED!"},
    {"CARTOON REELS", "THE LOST CARTOON VAULT", "THE SPLICER", "SHE CUTS EVERY PICTURE SHORT", "mm_boss3",
     {"TITLE CARD", "BARNYARD SHORT", "SPOOK SHORT", "FILM VAULT", "CUTTING ROOM", "THE SPLICING TABLE"}, {0, 40, 85, 125, 170, 210, LW},
     {"mm_level3", "mm_level3", "mm_level3", "mm_vault", "mm_vault", "mm_level3"}, 160, "THE SPLICER GETS CUT!", "SHE'S UNSPOOLING!"},
    {"THE INK WORKS", "WHERE THE INK IS MADE", "THE BIG PRESS", "IT PRINTS NOTHING BUT TROUBLE", "mm_boss4",
     {"LOADING BAY", "PRESS FLOOR", "INK VATS", "PIPE WORKS", "BOILER FURNACE", "THE PRESS ROOM"}, {0, 40, 85, 125, 170, 210, LW},
     {"mm_level4", "mm_level4", "mm_level4", "mm_pipes", "mm_pipes", "mm_level4"}, 180, "STOP THE PRESSES!", "IT'S OVERHEATING!"},
    {"LOST & FOUND", "EVERYTHING THE THEATRE EVER LOST", "THE COLLECTOR", "FINDERS KEEPERS", "mm_boss5",
     {"COAT CHECK", "LOST PROPERTY", "TOY CHEST", "LUGGAGE HALL", "DOLL HOSPITAL", "THE CLAIM DESK"}, {0, 40, 85, 125, 170, 210, LW},
     {"mm_level5", "mm_level5", "mm_level5", "mm_dolls", "mm_dolls", "mm_level5"}, 200, "CLAIM TICKET: VOID!", "HE WANTS IT ALL BACK!"},
    {"HALL OF MIRRORS", "THE STARS' DRESSING ROOMS", "MADAME VANITY", "THE FAIREST OF THEM ALL", "mm_boss7",
     {"STAGE DOOR", "WARDROBE", "MAKEUP ROW", "HALL OF MIRRORS", "THE STAR'S DRESSING ROOM", "THE VANITY"}, {0, 40, 85, 125, 170, 210, LW},
     {"mm_level7", "mm_level7", "mm_level7", "mm_mirrors", "mm_mirrors", "mm_level7"}, 220, "SEVEN YEARS BAD LUCK!", "SHE'S CRACKING UP!"},
    {"ROOFTOP MARQUEE", "UP ON THE ROOF, IN THE STORM", "THE HEADLINER", "YOUR NAME IN LIGHTS... FOREVER", "mm_boss8",
     {"FIRE ESCAPE", "THE ROOFTOP", "WATER TOWERS", "THE NEON SIGN", "LIGHTNING RODS", "THE MARQUEE"}, {0, 40, 85, 125, 170, 210, LW},
     {"mm_level8", "mm_level8", "mm_level8", "mm_storm", "mm_storm", "mm_level8"}, 240, "LIGHTS OUT FOR THE HEADLINER!", "HE'S OVERLOADING!"},
    {"ENCORE", "THE FINAL SHOW", "INKY UNBOUND", "THE SHOW MUST GO ON", "mm_boss6",
     {"BURNING LOBBY", "THE BALCONY", "ORCHESTRA PIT", "THE FLY LOFT", "CURTAIN CALL", "THE GRAND STAGE"}, {0, 40, 85, 125, 170, 210, LW},
     {"mm_level6", "mm_level6", "mm_level6", "mm_finale", "mm_finale", "mm_level6"}, 260, "THE CURTAIN FALLS ON UNCLE INKY!", "HIS HEART IS SHOWING! AIM FOR IT!"},
};
static int loop_no;   /* 0 = first run, 1 = the harder second run (beat it for the true ending) */
static int level_no;
/* front-of-house state (see the end of the file) */
static int paused, rush_on, show_deaths, show_grade, show_newbest;
static float show_time, rush_time;
static int difficulty(void);   /* 0 Matinee (easy), 1 Normal, 2 Midnight (hard) */
static void rumble_pad(int pad, float s, int ms);
static void rumble_all(float s, int ms);
static void show_cleared(void);
static Tex posters[8];
static const Uint8 OUTFIT_RGB[5][3] = {{255, 255, 255}, {150, 172, 255}, {128, 116, 140}, {255, 214, 120}, {206, 214, 232}};
static int over_sel, pause_sel, con_now[MAX_PADS], opt_back, credits_then;
static void award(int b);
static void step_badges(unsigned anyp);
static void draw_badges(void);
static void draw_toast(void);
static int show_hit, boss_hit;   /* got hurt during this show / this boss fight (badges) */
static void build_tracks(void);
static void start_intro(int then);
static void queue_show(int level, int keep);
static void menu_row(float y, const char *s, int sel, int dim, float scale);
static float last_shake;
static unsigned OK_BITS(void);
static int step_front(const Input pads[MAX_PADS], unsigned anyp, unsigned anyh);
static void step_pause(unsigned anyp);
static int draw_front(void);
static void draw_pause(void);
static void draw_options(void);
static void draw_clear_card(void);
static void draw_reels_world(float ox, float oy);
static void draw_tutorial_signs(float ox, float oy);
static void update_reels(void);
static int demo_on;   /* attract mode: the game playing itself */
static void game_save(void);
static void load_aftermath(int L);
static float after_len(void);
static void draw_rich(Font *f, const char *s, float x, float y, float scale, int centered, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
static float rich_width(Font *f, const char *s, float scale);
static void boss_entrance_update(void);
static void draw_boss(float ox, float oy);
static void iris_track(void);
static void draw_iris(void);
static void draw_aftermath(void);
static void open_slots(void);
static void vo(int chr, const char *line);
static const char *land_sfx(void);
static void to_press(void);
static void start_demo(void);
static Tex aftermath_tex, ego_tex, keyart_tex, iris_tex;
static float demo_t;
static Tex make_iris(void);
static void end_demo(void);
static Input demo_input(void);
static void step_slots(unsigned anyp);
static void draw_logo(void);
static void draw_press(void);
static void draw_slots(void);
static const char *const *ZONE_NAME = LEVELS[0].zone;
static const int *ZONE_X0 = LEVELS[0].x0;
static Tex zbg[6], zfg, zground; static int zart_show = -1;   /* the current show's art (see load_show_art) */
static int zone_of(float x) { int tx = (int)(x / TILE); for (int z = 0; z < 6; z++) if (tx < ZONE_X0[z + 1]) return z; return 5; }

/* Longer shows: the builders below are written in their original 240-column coordinates. While a show is built,
   RX() opens three 24-column gaps at quiet spots (plain floor, nothing nearby), and build_stretch() fills each gap
   with a new run made from that show's own hazards and enemies -- plus a hidden film reel. */
static int ins_at[3], n_ins, remap_on;
static unsigned char busy[OLD_LW + 1];   /* first pass: columns something was built on (no gap may open there) */
static int RX(int x) {
    if (!remap_on) return x;
    if (x >= OLD_LW) return x - (3 - n_ins) * STRETCH;   /* "LW - 1" in a builder: the (new) right edge */
    int k = 0; for (int i = 0; i < n_ins; i++) if (x >= ins_at[i]) k += STRETCH;
    return x + k;
}
static void mark_busy(int a, int b) { (void)a; (void)b; }   /* the map itself shows what's built; only invisible links need marking */
static void mark_span(int a, int b) { if (remap_on) return; if (a > b) { int t = a; a = b; b = t; } for (int x = a; x <= b; x++) if (x >= 0 && x <= OLD_LW) busy[x] = 1; }
static void put_raw(int x, int y, char c) { if (x >= 0 && x < LW && y >= 0 && y < LH) map[y][x] = c; }
static void put(int x, int y, char c) { put_raw(RX(x), y, c); }
static void block(int x0, int x1, int y0, int y1) { if (y0 < 13) mark_busy(x0, x1); x0 = RX(x0); x1 = RX(x1); for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) put_raw(x, y, '#'); }
static void plat(int x0, int x1, int y) { mark_busy(x0, x1); x0 = RX(x0); x1 = RX(x1); for (int x = x0; x <= x1; x++) put_raw(x, y, '='); }
static void pit(int x0, int x1) { mark_busy(x0, x1); x0 = RX(x0); x1 = RX(x1); for (int x = x0; x <= x1; x++) { put_raw(x, 13, '~'); put_raw(x, 14, '~'); } }
static void spawn(int type, int tx, int row) {
    mark_busy(tx - 1, tx + 1);
    tx = RX(tx);
    { static int noe = -1; if (noe < 0) noe = getenv("MM_NOENEMY") != NULL; if (noe && type < E_CRATE) return; }   /* tests: an empty level (route checks) */   /* (runtime spawns -- bosses summoning help -- pass through unchanged) */
    if (nen >= MAXE) return;
    Enemy *e = &en[nen++]; memset(e, 0, sizeof *e);
    e->type = type; e->alive = 1; e->x = tx * TILE + TILE / 2; e->y = row * TILE; e->face = -1;
    e->hp = type == E_USHER ? 3 : type == E_DOG ? 4 : type == E_BLOB ? 2 : type == E_MINIBLOB ? 1 :
            type == E_GOAT ? 4 : type == E_MEATBALL ? 3 : type == E_SLICE ? 2 :
            type == E_BOMB ? 2 : type == E_GHOST ? 3 : type == E_GLOVE ? 3 :
            type == E_IMP ? 2 : type == E_WORKER ? 5 : type == E_EEL ? 3 :
            type == E_SUITCASE ? 4 : type == E_SOLDIER ? 3 : type == E_UMBRELLA ? 2 : type == E_SHADOW ? 4 :
            type == E_MANNEQUIN ? 5 : type == E_PUFF ? 2 : type == E_MIRROR ? 4 : type == E_CROW ? 2 : type == E_BULB ? 3 : 1;
    if (loop_no && type < E_CRATE) e->hp += 1 + e->hp / 3;   /* second run: everything is tougher */
    if (difficulty() == 2 && type < E_CRATE) e->hp += 1 + e->hp / 4;
    e->timer = 1.0f + frand(); e->plat0 = 0; e->plat1 = LW * TILE;
    if (type == E_SLICE || type == E_UMBRELLA || type == E_PUFF || type == E_CROW) { e->plat0 = e->y - 30; e->timer = frand() * 3; }   /* plat0 = home altitude */
    if (type == E_GHOST) e->land = frand() * 4;   /* phase offset for fading */
    if (type == E_EEL) { e->y = FLOOR_Y; e->timer = 0.5f + frand(); }
}
static void breaker(int tx) { mark_busy(tx - 1, tx + 1); tx = RX(tx); if (nbreak < 8) breakers[nbreak++] = (Breaker){tx, 0}; put_raw(tx, 11, 'B'); }
static void checkpoint(int tx) { mark_busy(tx - 1, tx + 1); tx = RX(tx); if (ncheck < 12) checkpoints[ncheck++] = tx; put_raw(tx, 12, 'K'); }
static void closet(int tx) { mark_busy(tx - 1, tx + 1); put(tx, 11, 'C'); put(tx, 12, 'C'); }
static void drip(int tx) { mark_busy(tx - 1, tx + 1); tx = RX(tx); if (ndrip < 8) drips[ndrip++] = (Drip){tx, frand() * 2}; }

static void build_level1(void);
static void build_level2(void);
static void build_level3(void);
static void build_level4(void);
static void build_level5(void);
static void build_level6(void);
static void build_level7(void);
static void build_level8(void);
static void build_show(void) {
    switch (level_no) {
    case L_PIZZA: build_level2(); break;   case L_REELS: build_level3(); break;  case L_INK: build_level4(); break;
    case L_LOST: build_level5(); break;    case L_MIRROR: build_level7(); break; case L_ROOF: build_level8(); break;
    case L_ENCORE: build_level6(); break;  default: build_level1();
    }
}

/* ---- hidden film reels: three per show, one in each new stretch ---- */
typedef struct { int live; float x, y; } ReelSpot;
static ReelSpot reel_spot[3];
static int reels_run;   /* reels picked up during this visit to the show (bits) */
static int zone_x0_now[7];

static int is_flyer(int t) { return t == E_SLICE || t == E_UMBRELLA || t == E_PUFF || t == E_CROW || t == E_GHOST; }
static int is_walker(int t) {
    return t == E_USHER || t == E_BLOB || t == E_GOAT || t == E_MEATBALL || t == E_BOMB || t == E_GLOVE || t == E_IMP || t == E_WORKER ||
           t == E_SUITCASE || t == E_SOLDIER || t == E_SHADOW || t == E_MANNEQUIN || t == E_BULB;
}
/* the enemy this part of the show is known for (most common walker / flyer in that zone, else anywhere in the show) */
static int local_type(int x0, int x1, int flyer) {
    int count[64] = {0}, best = -1, bn = 0;
    for (int pass = 0; pass < 2 && best < 0; pass++)
        for (int i = 0; i < nen; i++) { int t = en[i].type, tx = (int)(en[i].x / TILE);
            if (t >= 64 || (flyer ? !is_flyer(t) : !is_walker(t))) continue;
            if (pass == 0 && (tx < x0 || tx >= x1)) continue;
            if (++count[t] > bn) { bn = count[t]; best = t; } }
    return best;
}
static int pit_safe(void) { return level_no != L_ENCORE; }   /* the Encore's flooded pit rules: no new pits there */
/* the stepping-stone material each show is built from (the ledge that holds the reel is always solid) */
static void conveyor(int x0, int x1, int y, char dir);
static void filmplat(int x0, int x1, int y);
static void blocks(int x0, int x1, int y);
static void neon(int x0, int x1, int y);
static void oven(int x0, int y);
static void saw(int tx, int ty);
static void crusher(int tx);
static void vent(int tx);
static void mousetrap(int tx);
static void mirror(int tx, int y0, int y1);
static void reflection(int tx, int line_tx);
static void rod(int tx, int y);
static void step_mat(int x0, int x1, int y) {
    switch (level_no) {
    case L_PIZZA: conveyor(x0, x1, y, '>'); break;
    case L_REELS: case L_ENCORE: filmplat(x0, x1, y); break;
    case L_LOST: blocks(x0, x1, y); break;
    case L_ROOF: neon(x0, x1, y); break;
    default: plat(x0, x1, y);
    }
}
/* each show's own trouble, placed past the reel so the route to it stays fair */
static void stretch_hazard(int k, int g) {
    int x = g + 20 + k % 2;
    switch (level_no) {
    case L_LOBBY: if (k == 0) { closet(x); } else if (k == 1) drip(g + 17); else closet(x + 1); break;
    case L_PIZZA: oven(x, 11); break;
    case L_REELS: saw(x, k == 1 ? 9 : 12); break;
    case L_INK: if (k == 1) vent(x); else crusher(x); break;
    case L_LOST: mousetrap(x); break;
    case L_MIRROR: if (k == 1) reflection(x + 2, x); else mirror(x, 10, 12); break;
    case L_ROOF: rod(x, 12); break;
    case L_ENCORE: if (k == 2) vent(x); else crusher(x); break;
    }
}
static void build_stretch(int k, int g, int zx0, int zx1) {
    int walker = local_type(zx0, zx1, 0), flyer = local_type(zx0, zx1, 1);
    if (walker < 0) walker = E_USHER;
    if (k == 0) {   /* THE CLIMB: hop a gap, then up two ledges to the reel (a crate sits in the way) */
        if (pit_safe()) pit(g + 3, g + 5);
        spawn(walker, g + 8, 13);
        step_mat(g + 10, g + 13, 10); plat(g + 15, g + 19, 7);
        spawn(E_CRATE, g + 16, 7);
        reel_spot[k] = (ReelSpot){1, (g + 18) * TILE + 24, 7 * TILE - 44};
        if (level_no == L_LOBBY) { int n0 = nen; spawn(E_DOG, g + 17, 7); if (nen > n0) { en[nen - 1].plat0 = (g + 15) * TILE + 30; en[nen - 1].plat1 = (g + 20) * TILE - 30; } }
        else if (flyer >= 0) spawn(flyer, g + 21, 7); else spawn(walker, g + 22, 13);
    } else if (k == 1) {   /* THE CROSSING: stepping stones over a long gap; the reel waits on a high ledge */
        if (pit_safe()) pit(g + 4, g + 12);
        step_mat(g + 5, g + 7, 10); step_mat(g + 9, g + 11, 9); plat(g + 14, g + 16, 6);
        reel_spot[k] = (ReelSpot){1, (g + 15) * TILE + 24, 6 * TILE - 44};
        if (flyer >= 0) spawn(flyer, g + 10, 5);
        spawn(walker, g + 19, 13); spawn(walker, g + 23, 13);
    } else {   /* THE TOWER: two steps up to the top of a stack, reel on the roof */
        spawn(walker, g + 3, 13);
        block(g + 6, g + 8, 11, 12); block(g + 11, g + 13, 9, 12);
        reel_spot[k] = (ReelSpot){1, (g + 12) * TILE + 24, 9 * TILE - 44};
        step_mat(g + 16, g + 19, 10); spawn(E_HATCRATE, g + 18, 10);
        if (flyer >= 0) spawn(flyer, g + 22, 6); else spawn(walker, g + 23, 13);
    }
    stretch_hazard(k, g);
}
static void build_level(void) {
    ZONE_NAME = LEVELS[level_no].zone;
    const int *ox = LEVELS[level_no].x0;
    /* pass 1: the show as written, noting every column something was built on */
    memset(busy, 0, sizeof busy); remap_on = 0; n_ins = 0;
    build_show();
    /* pick three quiet spots (plain floor, nothing within a few columns) -- one each in zones 1, 3 and 4 if possible */
    static const int prefer[5] = {1, 3, 4, 2, 0};
    for (int pi = 0; pi < 5 && n_ins < 3; pi++) {
        int z = prefer[pi]; if (level_no == L_ENCORE && z == 2) continue;
        int lo = ox[z] + 4, hi = ox[z + 1] - 4, mid = (lo + hi) / 2, found = -1;
        for (int d = 0; d <= (hi - lo) / 2 + 1 && found < 0; d++)
            for (int sgn = -1; sgn <= 1 && found < 0; sgn += 2) { int c = mid + sgn * d; if (c < lo || c > hi) continue;
                int ok = 1;
                if (busy[c - 1] || busy[c]) ok = 0;
                for (int x = c - 2; x <= c + 1 && ok; x++) {
                    for (int y = 1; y < 13 && ok; y++) if (map[y][x] != '.') ok = 0;
                    if (map[0][x] != '.' && map[0][x] != '#') ok = 0;   /* a plain ceiling just carries on over the gap */
                    if (map[13][x] != '#' || map[14][x] != '#') ok = 0; }
                for (int i = 0; i < n_ins && ok; i++) if (abs(ins_at[i] - c) < 10) ok = 0;
                if (ok) found = c; }
        if (found >= 0) ins_at[n_ins++] = found;
    }
    for (int i = 0; i < n_ins; i++) for (int j = i + 1; j < n_ins; j++) if (ins_at[j] < ins_at[i]) { int t = ins_at[i]; ins_at[i] = ins_at[j]; ins_at[j] = t; }
    /* pass 2: the real thing, with the gaps open */
    remap_on = 1;
    build_show();
    for (int z = 0; z < 7; z++) zone_x0_now[z] = RX(ox[z]);
    ZONE_X0 = zone_x0_now; arena_tx = RX(212);
    memset(reel_spot, 0, sizeof reel_spot);
    remap_on = 0;   /* the stretches are placed in final coordinates */
    for (int i = 0; i < n_ins; i++) { int g = ins_at[i] + i * STRETCH, z = zone_of(g * TILE);
        build_stretch(i, g, ZONE_X0[z], ZONE_X0[z + 1]); }
    if (getenv("MM_DUMPMAP")) { printf("show %d gaps at", level_no + 1); for (int i = 0; i < n_ins; i++) printf(" %d", ins_at[i]); printf(" arena %d\n", arena_tx);
        for (int y = 0; y < LH; y++) printf("%s\n", map[y]); }
    if (getenv("MM_CHECK") && n_ins < 3) printf("CHECK show%d: only %d stretches fit\n", level_no + 1, n_ins);
}
static void build_level1(void) {
    for (int y = 0; y < LH; y++) { memset(map[y], '.', LW); map[y][LW] = 0; }
    nen = nbreak = ndrip = ncheck = 0;
    block(0, LW - 1, 13, 14);
    block(0, 0, 0, 12); block(LW - 1, LW - 1, 0, 12);
    /* LOBBY */
    checkpoint(2);
    block(9, 10, 11, 12);
    plat(13, 18, 10); spawn(E_CRATE, 15, 10);
    spawn(E_USHER, 21, 13); spawn(E_USHER, 31, 13);
    breaker(26);
    block(34, 36, 12, 12);
    /* AUDITORIUM */
    checkpoint(41);
    block(44, 46, 12, 12); spawn(E_BLOB, 45, 12);
    block(47, 49, 11, 12);
    pit(50, 52);
    block(53, 55, 11, 12);
    plat(57, 63, 9); spawn(E_DOG, 60, 9);
    spawn(E_BLOB, 66, 13);
    pit(69, 70);
    plat(72, 76, 10); spawn(E_HATCRATE, 74, 10);
    spawn(E_USHER, 76, 13); spawn(E_BLOB, 81, 13);
    breaker(84);
    block(86, 88, 12, 12);
    /* BOILER ROOM */
    checkpoint(91);
    pit(95, 97); drip(99); spawn(E_BLOB, 101, 13);
    pit(103, 105); drip(108); spawn(E_BLOB, 108, 13);
    plat(110, 114, 9); pit(111, 113); spawn(E_BLOB, 116, 13); drip(117);
    spawn(E_CRATE, 118, 13); spawn(E_USHER, 120, 13);
    plat(121, 127, 9); spawn(E_DOG, 124, 9);
    breaker(128);
    /* BACKSTAGE: the Ink Fiend hunts here, hide in the prop closets */
    checkpoint(132);
    closet(138); block(142, 143, 12, 12); closet(147);
    spawn(E_USHER, 150, 13); pit(152, 153); closet(156);
    spawn(E_BLOB, 159, 13); block(161, 162, 11, 12); closet(165); closet(171);
    /* PROJECTION BOOTH: catwalks over an ink flood */
    checkpoint(176);
    pit(182, 191); plat(180, 184, 10); plat(186, 190, 8); spawn(E_DOG, 188, 8);
    spawn(E_USHER, 196, 13);
    plat(199, 204, 10); spawn(E_CRATE, 201, 10); spawn(E_BLOB, 200, 13);
    breaker(206);
    /* MAIN STAGE (boss arena) */
    checkpoint(209);
    for (int i = 0; i < nen; i++) {   /* dogs patrol their platform */
        Enemy *e = &en[i]; if (e->type != E_DOG) continue;
        int row = (int)(e->y / TILE), tx = (int)(e->x / TILE), a = tx, b = tx;
        while (a > 0 && map[row][a - 1] == '=') a--;
        while (b < LW - 1 && map[row][b + 1] == '=') b++;
        e->plat0 = a * TILE + 30; e->plat1 = (b + 1) * TILE - 30;
    }
}


/* SHOW 2: PIZZA PARTY. '>' '<' conveyor belts, 'O' brick ovens that blast fire, pits are hot cheese */
static void conveyor(int x0, int x1, int y, char dir) { mark_busy(x0, x1); x0 = RX(x0); x1 = RX(x1); for (int x = x0; x <= x1; x++) put_raw(x, y, dir); }
static void oven(int x0, int y) { mark_busy(x0 - 1, x0 + 2); x0 = RX(x0); for (int x = x0; x < x0 + 2; x++) for (int yy = y; yy <= 12; yy++) put_raw(x, yy, 'O'); }
static void build_level2(void) {
    for (int y = 0; y < LH; y++) { memset(map[y], '.', LW); map[y][LW] = 0; }
    nen = nbreak = ndrip = ncheck = 0;
    block(0, LW - 1, 13, 14);
    block(0, 0, 0, 12); block(LW - 1, LW - 1, 0, 12);
    /* PARTY ROOM: long party tables, a goat band and swooping pizza bats */
    checkpoint(2);
    plat(8, 14, 10); spawn(E_CRATE, 11, 10);
    spawn(E_GOAT, 19, 13); spawn(E_SLICE, 24, 7);
    plat(26, 32, 10); spawn(E_SLICE, 33, 6); spawn(E_GOAT, 36, 13);
    breaker(30);
    /* DINING HALL: hot cheese pits and rolling meatballs */
    checkpoint(41);
    spawn(E_MEATBALL, 50, 13); pit(54, 56); plat(53, 57, 10);
    spawn(E_SLICE, 60, 7); block(62, 64, 12, 12); spawn(E_MEATBALL, 68, 13);
    pit(71, 73); spawn(E_GOAT, 77, 13); plat(78, 82, 10); spawn(E_HATCRATE, 80, 10);
    /* PIZZA COUNTER: climb the counter, goats on top */
    checkpoint(86);
    block(92, 103, 11, 12); spawn(E_GOAT, 98, 11); spawn(E_SLICE, 96, 6);
    plat(105, 109, 9); spawn(E_MEATBALL, 112, 13); spawn(E_CRATE, 107, 9);
    block(114, 116, 12, 12); spawn(E_SLICE, 119, 7); breaker(121);
    /* KITCHEN: fire ovens and conveyor belts */
    checkpoint(126);
    oven(131, 11); conveyor(134, 140, 12, '>'); oven(142, 11); spawn(E_MEATBALL, 138, 12);
    spawn(E_SLICE, 145, 7); conveyor(147, 153, 12, '<'); oven(155, 11); spawn(E_GOAT, 151, 12);
    pit(158, 160); plat(157, 161, 10); oven(163, 11); spawn(E_CRATE, 166, 13); breaker(167);
    /* DELIVERY DOCK: conveyors over a cheese vat */
    checkpoint(171);
    pit(176, 196); conveyor(175, 181, 11, '>'); conveyor(184, 189, 10, '<'); conveyor(192, 197, 11, '>');
    spawn(E_SLICE, 183, 6); spawn(E_SLICE, 191, 5); spawn(E_GOAT, 201, 13); spawn(E_MEATBALL, 205, 13);
    breaker(203);
    /* THE RAT'S DEN (boss) */
    checkpoint(209);
    for (int i = 0; i < nen; i++) en[i].plat1 = LW * TILE;
}
/* SHOW 3: CARTOON REELS. 'F' film-frame platforms blink in and out, 'S' sprocket saws */
static void filmplat(int x0, int x1, int y) { mark_busy(x0, x1); x0 = RX(x0); x1 = RX(x1); for (int x = x0; x <= x1; x++) put_raw(x, y, 'F'); }
static void saw(int tx, int ty) { mark_busy(tx - 2, tx + 2); put(tx, ty, 'S'); }
static void build_level3(void) {
    for (int y = 0; y < LH; y++) { memset(map[y], '.', LW); map[y][LW] = 0; }
    nen = nbreak = ndrip = ncheck = 0;
    block(0, LW - 1, 13, 14);
    block(0, 0, 0, 12); block(LW - 1, LW - 1, 0, 12);
    /* TITLE CARD: hopping bombs, first blinking frames */
    checkpoint(2);
    spawn(E_BOMB, 14, 13); plat(17, 22, 10); spawn(E_CRATE, 20, 10);
    spawn(E_BOMB, 26, 13); filmplat(29, 33, 9); spawn(E_GLOVE, 36, 13); breaker(31);
    /* BARNYARD SHORT: gloves scuttle over the fences, ink ponds */
    checkpoint(41);
    block(48, 50, 12, 12); spawn(E_GLOVE, 55, 13); pit(58, 61); filmplat(57, 62, 10);
    spawn(E_BOMB, 66, 13); spawn(E_BOMB, 69, 13); saw(72, 12); block(75, 77, 11, 12); spawn(E_GLOVE, 76, 11);
    plat(79, 83, 9); spawn(E_HATCRATE, 81, 9);
    /* SPOOK SHORT: film ghosts fade in and out */
    checkpoint(86);
    spawn(E_GHOST, 94, 9); filmplat(97, 101, 10); saw(99, 7); spawn(E_GHOST, 106, 8);
    pit(108, 111); filmplat(107, 112, 10); spawn(E_GLOVE, 115, 13); spawn(E_GHOST, 119, 8); breaker(121);
    /* FILM VAULT: towers of film cans, sprocket saws, frames that skip */
    checkpoint(126);
    block(131, 133, 11, 12); saw(135, 12); block(137, 139, 10, 12); spawn(E_BOMB, 138, 10);
    filmplat(141, 145, 8); saw(143, 5); spawn(E_GHOST, 147, 8); block(149, 151, 11, 12); saw(153, 12);
    spawn(E_GLOVE, 157, 13); filmplat(159, 163, 10); spawn(E_CRATE, 161, 10); spawn(E_BOMB, 165, 13); breaker(167);
    /* CUTTING ROOM: an ink trough crossed on skipping frames, saws swinging */
    checkpoint(171);
    pit(175, 197); filmplat(175, 178, 11); filmplat(181, 184, 10); saw(182, 6); filmplat(187, 190, 11);
    filmplat(193, 197, 10); saw(195, 6); spawn(E_GHOST, 185, 7); spawn(E_GHOST, 193, 6);
    spawn(E_GLOVE, 201, 13); spawn(E_BOMB, 205, 13); breaker(203);
    /* THE SPLICING TABLE (boss) */
    checkpoint(209);
    for (int i = 0; i < nen; i++) en[i].plat1 = LW * TILE;
}
/* SHOW 4: THE INK WORKS. 'X' ceiling crushers, 'V' steam vents that launch you, '~' ink vats with eels in them */
static void crusher(int tx) { mark_busy(tx - 2, tx + 2); put(tx, 0, 'X'); }
static void vent(int tx) { mark_busy(tx - 1, tx + 1); put(tx, 12, 'V'); }
static void build_level4(void) {
    for (int y = 0; y < LH; y++) { memset(map[y], '.', LW); map[y][LW] = 0; }
    nen = nbreak = ndrip = ncheck = 0;
    block(0, LW - 1, 13, 14);
    block(1, LW - 2, 0, 0);   /* factory ceiling for the crushers to hang from */
    block(0, 0, 0, 12); block(LW - 1, LW - 1, 0, 12);
    /* LOADING BAY: crates, imps, the first steam vent */
    checkpoint(2);
    spawn(E_IMP, 14, 13); block(17, 19, 12, 12); spawn(E_CRATE, 18, 12);
    vent(23); plat(22, 27, 6); spawn(E_IMP, 25, 6); spawn(E_WORKER, 33, 13); breaker(30);
    /* PRESS FLOOR: crushers stamping, wind-up workers charging */
    checkpoint(41);
    crusher(48); spawn(E_WORKER, 54, 13); crusher(58); plat(61, 66, 9); spawn(E_IMP, 64, 9);
    crusher(70); spawn(E_WORKER, 75, 13); vent(78); plat(77, 82, 5); spawn(E_HATCRATE, 80, 5);
    /* INK VATS: eels in the ink, catwalks over the top */
    checkpoint(86);
    pit(91, 96); spawn(E_EEL, 93, 13); plat(90, 97, 9); spawn(E_IMP, 95, 9);
    pit(101, 106); spawn(E_EEL, 103, 13); plat(100, 107, 9);
    spawn(E_WORKER, 112, 13); pit(115, 119); spawn(E_EEL, 117, 13); plat(114, 120, 9); breaker(122);
    /* PIPE WORKS: vents launch you up to the high pipes */
    checkpoint(126);
    block(131, 133, 10, 12); vent(135); plat(134, 140, 4); spawn(E_IMP, 138, 4); crusher(143);
    spawn(E_WORKER, 147, 13); vent(151); plat(150, 157, 5); spawn(E_IMP, 155, 5); pit(152, 156); spawn(E_EEL, 154, 13);
    spawn(E_CRATE, 161, 13); crusher(164); breaker(167);
    /* BOILER FURNACE: crushers over an ink trough, eels below */
    checkpoint(171);
    pit(175, 196); plat(175, 179, 10); crusher(177); plat(182, 186, 9); spawn(E_EEL, 181, 13); plat(189, 193, 10); crusher(191);
    spawn(E_EEL, 188, 13); spawn(E_EEL, 195, 13); plat(195, 198, 9);
    spawn(E_WORKER, 202, 13); spawn(E_IMP, 206, 13); breaker(204);
    /* THE PRESS ROOM (boss) */
    checkpoint(209);
    for (int i = 0; i < nen; i++) en[i].plat1 = LW * TILE;
}
/* SHOW 5: LOST & FOUND. 'U' toy-block platforms crumble after you land, 'M' giant mousetraps snap */
static float crumble_t[LH][LW], regrow_t[LH][LW], trap_t[LW];
static void blocks(int x0, int x1, int y) { mark_busy(x0, x1); x0 = RX(x0); x1 = RX(x1); for (int x = x0; x <= x1; x++) put_raw(x, y, 'U'); }
static void mousetrap(int tx) { mark_busy(tx - 1, tx + 1); put(tx, 12, 'M'); }
static void build_level5(void) {
    for (int y = 0; y < LH; y++) { memset(map[y], '.', LW); map[y][LW] = 0; }
    memset(crumble_t, 0, sizeof crumble_t); memset(regrow_t, 0, sizeof regrow_t); memset(trap_t, 0, sizeof trap_t);
    nen = nbreak = ndrip = ncheck = 0;
    block(0, LW - 1, 13, 14);
    block(0, 0, 0, 12); block(LW - 1, LW - 1, 0, 12);
    /* COAT CHECK: suitcases hop, umbrellas swoop */
    checkpoint(2);
    spawn(E_SUITCASE, 14, 13); plat(17, 22, 10); spawn(E_CRATE, 20, 10); spawn(E_UMBRELLA, 24, 7);
    mousetrap(27); spawn(E_SOLDIER, 32, 13); breaker(30);
    /* LOST PROPERTY: shelves to climb, soldiers below */
    checkpoint(41);
    block(46, 48, 11, 12); spawn(E_SOLDIER, 52, 13); spawn(E_SOLDIER, 55, 13); mousetrap(58); plat(57, 63, 9); spawn(E_UMBRELLA, 60, 6);
    spawn(E_SUITCASE, 67, 13); block(70, 72, 12, 12); mousetrap(75); plat(78, 83, 9); spawn(E_HATCRATE, 81, 9);
    /* TOY CHEST: toy-block bridges that crumble under you */
    checkpoint(86);
    pit(91, 96); blocks(90, 97, 10); spawn(E_UMBRELLA, 94, 6); spawn(E_SUITCASE, 100, 13);
    pit(104, 110); blocks(103, 106, 10); blocks(108, 111, 9); spawn(E_UMBRELLA, 107, 5);
    spawn(E_SOLDIER, 115, 13); mousetrap(118); breaker(122);
    /* LUGGAGE HALL: towers of trunks, suitcases leaping off them */
    checkpoint(126);
    block(130, 132, 11, 12); spawn(E_SUITCASE, 131, 11); block(135, 138, 9, 12); spawn(E_SOLDIER, 136, 9); mousetrap(140);
    block(142, 144, 11, 12); spawn(E_SUITCASE, 143, 11); spawn(E_UMBRELLA, 148, 6); blocks(150, 155, 9); pit(150, 155);
    spawn(E_SOLDIER, 159, 13); spawn(E_CRATE, 162, 13); mousetrap(164); breaker(167);
    /* DOLL HOSPITAL: crumbling blocks over an ink drain, umbrellas everywhere */
    checkpoint(171);
    pit(175, 196); blocks(175, 178, 10); blocks(181, 184, 9); blocks(187, 190, 10); blocks(193, 197, 9);
    spawn(E_UMBRELLA, 180, 5); spawn(E_UMBRELLA, 189, 5); spawn(E_UMBRELLA, 196, 6);
    spawn(E_SUITCASE, 201, 13); spawn(E_SOLDIER, 206, 13); breaker(203);
    /* THE CLAIM DESK (boss) */
    checkpoint(209);
    for (int i = 0; i < nen; i++) en[i].plat1 = LW * TILE;
}
/* SHOW 6: ENCORE. Every hazard and enemy comes back for the final show, plus ink-shadow understudies of you.
   The orchestra pit floods with ink in waves; the fly loft drops sandbags ('X' drawn as sandbags here) */
static void build_level6(void) {
    for (int y = 0; y < LH; y++) { memset(map[y], '.', LW); map[y][LW] = 0; }
    nen = nbreak = ndrip = ncheck = 0;
    block(0, LW - 1, 13, 14);
    block(0, 0, 0, 12); block(LW - 1, LW - 1, 0, 12);
    /* BURNING LOBBY: old friends from the first show */
    checkpoint(2);
    spawn(E_USHER, 14, 13); spawn(E_BLOB, 19, 13); plat(21, 26, 10); spawn(E_CRATE, 23, 10); spawn(E_SLICE, 27, 7);
    spawn(E_SHADOW, 33, 13); breaker(30); drip(17); drip(29);
    /* THE BALCONY: tiers of seats to climb, goats and soldiers */
    checkpoint(41);
    block(46, 49, 12, 12); block(50, 53, 11, 12); spawn(E_GOAT, 52, 11); block(54, 57, 10, 12); spawn(E_SOLDIER, 56, 10);
    filmplat(59, 63, 8); spawn(E_GHOST, 64, 7); block(66, 69, 11, 12); spawn(E_DOG, 72, 13); spawn(E_SHADOW, 76, 13);
    plat(78, 83, 9); spawn(E_HATCRATE, 81, 9);
    /* ORCHESTRA PIT: the ink floods in waves; get up on the music stands */
    checkpoint(86);
    plat(89, 93, 10); plat(96, 100, 9); spawn(E_IMP, 98, 9); plat(103, 107, 10); spawn(E_UMBRELLA, 105, 6);
    plat(110, 114, 9); spawn(E_GLOVE, 112, 9); plat(116, 120, 10); spawn(E_BOMB, 118, 10); breaker(122);
    /* THE FLY LOFT: sandbags drop on ropes, steam vents up to the rigging */
    checkpoint(126);
    crusher(132); spawn(E_WORKER, 136, 13); vent(140); plat(139, 145, 5); spawn(E_SUITCASE, 143, 5); crusher(148);
    spawn(E_SHADOW, 152, 13); saw(156, 12); crusher(160); spawn(E_MEATBALL, 163, 13); breaker(167);
    /* CURTAIN CALL: one last gauntlet over the ink */
    checkpoint(171);
    pit(175, 196); filmplat(175, 178, 10); blocks(181, 184, 9); plat(187, 190, 10); filmplat(193, 197, 9);
    spawn(E_EEL, 180, 13); spawn(E_EEL, 189, 13); spawn(E_GHOST, 185, 6); spawn(E_UMBRELLA, 194, 5);
    spawn(E_SHADOW, 201, 13); spawn(E_SHADOW, 206, 13); breaker(203);
    /* THE GRAND STAGE (final boss) */
    checkpoint(209);
    for (int i = 0; i < nen; i++) en[i].plat1 = LW * TILE;
}
static float film_phase(int tx);
static int neon_on(int tx);
static int film_on(int tx) { return film_phase(tx) < 2.1f; }
static int solid_at(int tx, int ty) { if (tx < 0 || tx >= LW) return 1; if (ty < 0 || ty >= LH) return 0; char c = map[ty][tx]; return c == '#' || c == 'O' || c == '>' || c == '<' || c == 'Y'; }
static int oneway_at(int tx, int ty) { if (tx < 0 || tx >= LW || ty < 0 || ty >= LH) return 0; char c = map[ty][tx]; return c == '=' || c == 'U' || (c == 'F' && film_on(tx)) || (c == 'n' && neon_on(tx)); }

static int move_box(float *x, float *y, float *vx, float *vy, float w, float h, int drop);
/* standing on a conveyor belt: it carries you along */
static void conveyor_carry(float *x, float *y, float w, float h) {
    int tx = (int)(*x / TILE), ty = (int)((*y + 2) / TILE);
    if (tx < 0 || tx >= LW || ty < 0 || ty >= LH) return;
    char c = map[ty][tx]; if (c != '>' && c != '<') return;
    float cv = c == '>' ? 150 : -150, vy0 = 0; move_box(x, y, &cv, &vy0, w, h, 0);
}
/* x = centre, y = feet. returns 1 = landed, 2 = hit a wall */
static int move_box(float *x, float *y, float *vx, float *vy, float w, float h, int drop) {
    int res = 0;
    float nx = *x + *vx * DT;
    float top = *y - h, bot = *y - 1;
    int ty0 = (int)floorf(top / TILE), ty1 = (int)floorf(bot / TILE);
    if (*vx > 0) { int tx = (int)floorf((nx + w / 2) / TILE); for (int ty = ty0; ty <= ty1; ty++) if (solid_at(tx, ty)) { nx = tx * TILE - w / 2 - 0.01f; *vx = 0; res |= 2; break; } }
    else if (*vx < 0) { int tx = (int)floorf((nx - w / 2) / TILE); for (int ty = ty0; ty <= ty1; ty++) if (solid_at(tx, ty)) { nx = (tx + 1) * TILE + w / 2 + 0.01f; *vx = 0; res |= 2; break; } }
    *x = nx;
    float ny = *y + *vy * DT;
    int tx0 = (int)floorf((*x - w / 2 + 1) / TILE), tx1 = (int)floorf((*x + w / 2 - 1) / TILE);
    if (*vy >= 0) {
        int ty = (int)floorf(ny / TILE), pty = (int)floorf((*y - 0.01f) / TILE);
        for (int tx = tx0; tx <= tx1; tx++) if (solid_at(tx, ty) || (!drop && oneway_at(tx, ty) && pty < ty)) { ny = ty * TILE; *vy = 0; res |= 1; break; }
    } else {
        int ty = (int)floorf((ny - h) / TILE);
        for (int tx = tx0; tx <= tx1; tx++) if (solid_at(tx, ty)) { ny = (ty + 1) * TILE + h; *vy = 0; break; }
    }
    *y = ny;
    return res;
}
static int overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return fabsf(ax - bx) * 2 < aw + bw && ay - ah < by && by - bh < ay;   /* boxes: centre x, feet y */
}

/* ------------------------------------------------------------ players */
enum { W_NAIL, W_RIVET, W_TORCH, W_PLUNGER, NWEAP };
static const char *WEAP_NAME[NWEAP] = {"NAIL GUN", "RIVET SPREAD", "WELDING TORCH", "PLUNGER LAUNCHER"};
typedef struct {
    int active, pad, alive, gear, lives, face, onground, hidden, crouch, weapon, aim_dx, aim_dy, shooting, closet_tx, chr;
    float x, y, vx, vy, inv, cool, anim, dead_t, shoot_t, bob, safe_x, safe_y;   /* safe: last solid ground (Matinee pits) */
    float land_t, jump_t, turn_t, hurt_t, idle_t, step_t; int was_ground, prev_face;   /* cartoon squash & stretch, flinch, idle */
} Player;
static Player pl[2];
static int mode;            /* 0 solo (GamePad does everything), 1 security office co-op */
static int op_pad;          /* who runs the cameras */
static int score;

/* projectiles */
enum { P_NAIL, P_RIVET, P_FLAME, P_PLUNGER, P_LIGHT, P_NOTE, P_TICKET, P_DRIP, P_CONFETTI, P_PIZZA, P_CHEESE, P_FILM, P_REEL, P_INKBLOB, P_SHEET, P_CORK, P_SHOE, P_LIPSTICK, P_SHARD, P_KISS, P_SPARK, P_LETTER, P_BULB };
typedef struct { int type, alive, enemy, dmg, bounces, last_hit; float x, y, vx, vy, life, grav, r; } Proj;
#define MAXP 160
static Proj pr[MAXP];
enum { PK_DOT, PK_BLOOD, PK_SMOKE, PK_EMBER, PK_CASING, PK_DEBRIS };
typedef struct { float x, y, vx, vy, life, max, size, grav, ang, vang; Uint8 r, g, b, add, kind; } Part;
#define MAXPART 500
static Part parts[MAXPART];

/* pickups (from crates) */
typedef struct { int alive, kind; float x, y, vy, t; } Pickup;   /* kind 0..3 weapon (cycles), 10 = hard hat */
static Pickup pick[8];

/* world state */
static float power = 100, lit[6], camx, shake, msg_t, warn_t, cam_flash;
static char msg[64];
static int p1_char = -1;   /* runner picked on the title screen; player 2 gets the other one */
static const char *CHAR_NAME[NCHARS] = {"JOE", "ROSIE"};
static int scene, menu_sel = 1, cam_sel = 0, quit_req, frame_no, darkness_on;
enum { SC_TITLE, SC_PLAY, SC_OVER, SC_CLEAR, SC_END, SC_LOAD, SC_SELECT, SC_OPTIONS, SC_EXTRAS, SC_GALLERY, SC_JUKEBOX, SC_WARDROBE, SC_INTRO, SC_CREDITS, SC_RUSHDONE, SC_LOGO, SC_PRESS, SC_SLOTS, SC_BADGES };
static float scene_t, ping_cool, op_idle;
static float film_phase(int tx) { return fmodf(scene_t + (tx / 5) * 1.1f, 3.2f); }
static float negative_t;   /* the Splicer's frame-skip flash */

/* the Ink Fiend */
static struct { int state; float x, y, t, cycle; int seen; } fiend;   /* 0 asleep, 1 warning, 2 hunting, 3 retreating */
/* boss */
static struct {
    int active, state, hp, maxhp, pose, flash, slams, intro, dead, summoned, face;
    float x, y, t, timer, targetx, sweepx, bob, vy;
} boss;
static int arena_locked;

static void message(const char *s, float t) { snprintf(msg, sizeof msg, "%s", s); msg_t = t; }

static void part(float x, float y, float vx, float vy, float life, float size, float grav, Uint8 r, Uint8 g, Uint8 b, int add) {
    for (int i = 0; i < MAXPART; i++) if (parts[i].life <= 0) {
        parts[i] = (Part){x, y, vx, vy, life, life, size, grav, 0, 0, r, g, b, (Uint8)add, PK_DOT}; return;
    }
}
static void ink_burst(float x, float y, int n, float spd) {
    for (int i = 0; i < n; i++) { float a = frand() * 6.283f, s = spd * (0.3f + frand());
        part(x, y, cosf(a) * s, sinf(a) * s - 120, 0.5f + frand() * 0.5f, 6 + frand() * 10, 900, 12, 10, 10, 0); }
}
static void spark_burst(float x, float y, int n, Uint8 r, Uint8 g, Uint8 b) {
    for (int i = 0; i < n; i++) { float a = frand() * 6.283f, s = 150 + frand() * 300;
        part(x, y, cosf(a) * s, sinf(a) * s, 0.25f + frand() * 0.3f, 3 + frand() * 4, 300, r, g, b, 1); }
}

/* ---------------- effects: kinds of particle, gibs, splatter decals, shockwaves, hit-pause ---------------- */
static Part *part_k(int kind, float x, float y, float vx, float vy, float life, float size, float grav, Uint8 r, Uint8 g, Uint8 b, int add) {
    for (int i = 0; i < MAXPART; i++) if (parts[i].life <= 0) {
        parts[i] = (Part){x, y, vx, vy, life, life, size, grav, frand() * 360, (frand() - 0.5f) * 900, r, g, b, (Uint8)add, (Uint8)kind}; return &parts[i];
    }
    return NULL;
}
static void blood_spurt(float x, float y, float dirx, int n, float spd) {   /* animatronic "blood": red oil mixed with ink */
    for (int i = 0; i < n; i++) {
        float a = atan2f(-0.6f, dirx) + (frand() - 0.5f) * 1.6f, v = spd * (0.4f + frand());
        int ink = rand() % 3 == 0;
        part_k(PK_BLOOD, x, y, cosf(a) * v, sinf(a) * v, 1.4f, 3 + frand() * 6, 1500, ink ? 14 : 150 + rand() % 40, ink ? 10 : 12, ink ? 12 : 18, 0);
    }
}
static void smoke(float x, float y, int n, Uint8 shade) {
    for (int i = 0; i < n; i++) part_k(PK_SMOKE, x + (frand() - 0.5f) * 20, y, (frand() - 0.5f) * 60, -40 - frand() * 70, 0.9f + frand() * 0.6f, 10 + frand() * 10, -20, shade, shade, shade, 0);
}
static void embers(float x, float y, int n) {
    for (int i = 0; i < n; i++) part_k(PK_EMBER, x, y, (frand() - 0.5f) * 220, -100 - frand() * 220, 0.5f + frand() * 0.6f, 3 + frand() * 3, 200, 255, 140 + rand() % 80, 40, 1);
}

typedef struct { Tex *t; SDL_Rect src; float x, y, vx, vy, ang, vang, life, w, h; int flip, bleed, rest; } Gib;
#define MAXGIB 140
static Gib gibs[MAXGIB]; static int gib_next;
static void gib(Tex *t, SDL_Rect src, float x, float y, float vx, float vy, float w, float h, int flip, int bleed) {
    if (!t || !t->tex) return;
    Gib *g = &gibs[gib_next]; gib_next = (gib_next + 1) % MAXGIB;
    *g = (Gib){t, src, x, y, vx, vy, frand() * 360, (frand() - 0.5f) * 1100, 6.0f + frand() * 2, w, h, flip, bleed, 0};
}
static void gore_part2(int idx, float x, float y, float spd, int bleed) {
    Tex *t = &A.gore2[idx]; if (!t->tex) return;
    float a = -1.57f + (frand() - 0.5f) * 2.4f, v = spd * (0.5f + frand());
    gib(t, (SDL_Rect){0, 0, t->w, t->h}, x, y, cosf(a) * v, sinf(a) * v, (float)t->w, (float)t->h, rand() & 1, bleed);
}
static void gore_part(int idx, float x, float y, float spd, int bleed) {
    Tex *t = &A.gore[idx]; if (!t->tex) return;
    float a = -1.57f + (frand() - 0.5f) * 2.4f, v = spd * (0.5f + frand());
    gib(t, (SDL_Rect){0, 0, t->w, t->h}, x, y, cosf(a) * v, sinf(a) * v, (float)t->w, (float)t->h, rand() & 1, bleed);
}
/* chop the sprite into a grid of chunks that fly apart */
static void shatter(Tex *t, float cx, float feet, int flip, int cols, int rows, float spd, int bleed) {
    if (!t || !t->tex) return;
    int cw = t->w / cols, ch = t->h / rows;
    for (int r = 0; r < rows; r++) for (int c = 0; c < cols; c++) {
        SDL_Rect src = {c * cw, r * ch, cw, ch};
        float lx = flip ? (t->w - (c + 0.5f) * cw) : (c + 0.5f) * cw;
        float x = cx - t->w / 2.0f + lx, y = feet - t->h + (r + 0.5f) * ch;
        float dx = x - cx, dy = y - (feet - t->h * 0.5f);
        gib(t, src, x, y, dx * 4 + (frand() - 0.5f) * spd, dy * 3 - spd * (0.6f + frand() * 0.6f), (float)cw, (float)ch, flip, bleed && (rand() % 2));
    }
}

typedef struct { float x, y, rx, ry, ang; int tex; Uint8 r, g, b, a; } Splat;
#define MAXSPLAT 260
static Splat splats[MAXSPLAT]; static int splat_next;
static void splat(float x, float y, float size, int floor, Uint8 r, Uint8 g, Uint8 b) {
    Splat *s = &splats[splat_next]; splat_next = (splat_next + 1) % MAXSPLAT;
    int tex = (size > 9 && A.gore[8].tex) ? 8 + (rand() & 1) : -1;
    *s = (Splat){x, y, size * (floor ? 2.2f : 1.0f), size * (floor ? 0.5f : 1.0f), floor ? 0 : frand() * 360, tex, r, g, b, (Uint8)(200 + rand() % 55)};
}

typedef struct { float x, y, r, max, life; Uint8 cr, cg, cb; } Ring;
static Ring rings[16];
static void ring(float x, float y, float max, Uint8 r, Uint8 g, Uint8 b) {
    for (int i = 0; i < 16; i++) if (rings[i].life <= 0) { rings[i] = (Ring){x, y, 10, max, 0.45f, r, g, b}; return; }
}
static int hitstop;
static Proj *proj(int type, float x, float y, float vx, float vy, int enemy, int dmg) {
    for (int i = 0; i < MAXP; i++) if (!pr[i].alive) {
        Proj *p = &pr[i]; memset(p, 0, sizeof *p);
        p->type = type; p->alive = 1; p->x = x; p->y = y; p->vx = vx; p->vy = vy; p->enemy = enemy; p->dmg = dmg;
        p->life = 2.5f; p->r = 8; p->last_hit = -1; return p;
    }
    return NULL;
}

static void reset_player(Player *p, int full) {
    int tx = checkpoints[cur_check];
    p->x = tx * TILE + TILE / 2 + (p == &pl[1] ? 40 : 0); p->y = FLOOR_Y - 2; p->vx = p->vy = 0;
    p->alive = 1; p->gear = 3; p->inv = 2.0f; p->hidden = 0; p->face = 1; p->dead_t = 0; p->safe_x = p->x; p->safe_y = p->y;
    p->prev_face = 1; p->was_ground = 1; p->land_t = p->jump_t = p->turn_t = p->hurt_t = p->idle_t = 0;
    if (p1_char < 0) p1_char = getenv("MM_CHAR") ? atoi(getenv("MM_CHAR")) % NCHARS : save.runner % NCHARS;
    p->chr = p == &pl[0] ? p1_char : (p1_char + 1) % NCHARS;
    if (full) { p->lives = difficulty() == 0 ? 5 : difficulty() == 2 ? 2 : 3; p->weapon = getenv("MM_WEAPON") ? atoi(getenv("MM_WEAPON")) % NWEAP : W_NAIL; }
}

static void load_show_art(void);
static void start_level(void) {
    build_level(); load_show_art();
    memset(pr, 0, sizeof pr); memset(parts, 0, sizeof parts); memset(pick, 0, sizeof pick);
    memset(gibs, 0, sizeof gibs); memset(splats, 0, sizeof splats); memset(rings, 0, sizeof rings); hitstop = 0;
    memset(&fiend, 0, sizeof fiend); memset(&boss, 0, sizeof boss); memset(lit, 0, sizeof lit);
    arena_locked = 0; power = 100; cur_check = 0; score = 0; cam_sel = 0;
    reset_player(&pl[0], 1); pl[0].active = 1;
    if (mode == 1) { pl[0].pad = 1; op_pad = 0; } else { pl[0].pad = 0; op_pad = 0; }
    pl[1].active = 0;
    camx = 0; scene = SC_PLAY; scene_t = 0;
    if (getenv("MM_TX")) { pl[0].x = atoi(getenv("MM_TX")) * TILE; pl[0].safe_x = pl[0].x; camx = clampf(pl[0].x - SCREEN_W * 0.4f, 0, LW * TILE - SCREEN_W); }   /* tests */
    { char b[64]; snprintf(b, sizeof b, "SHOW %d: %s", level_no + 1, LEVELS[level_no].title); message(b, 2.5f); }
    music_play(LEVELS[level_no].music[0], 1);
}

static int god_mode(void) { static int g = -1; if (g < 0) g = getenv("MM_GOD") != NULL; return g || demo_on; }   /* tests: invincible runner */
static void reset_player(Player *p, int full);
static void hurt_player(Player *p, float from_x) {
    if (!p->alive || p->inv > 0 || p->hidden) return;
    if (god_mode()) { p->inv = 1.6f; p->vx = (p->x < from_x ? -1 : 1) * 260; p->vy = -420; return; }
    p->gear--; p->inv = 1.6f; shake = 8;
    p->vx = (p->x < from_x ? -1 : 1) * 260; p->vy = -420;
    sfx("hurt"); rumble_pad(p->pad, p->gear <= 0 ? 1.0f : 0.6f, p->gear <= 0 ? 450 : 220);
    p->hurt_t = 0.45f; show_hit = 1; if (boss.active) boss_hit = 1;
    if (p->gear <= 0) { vo(p->chr, "ko"); sfx("dizzy"); } else if (rand() % 2 == 0) vo(p->chr, "hurt");
    if (p->gear == 2) { for (int i = 0; i < 6; i++) part(p->x, p->y - 140, (frand() - 0.5f) * 300, -300 - frand() * 200, 0.9f, 9, 900, 230, 180, 50, 0); message("THERE GOES THE HARD HAT!", 1.4f); }
    if (p->gear == 1) { for (int i = 0; i < 10; i++) part(p->x, p->y - 70, (frand() - 0.5f) * 400, -300 - frand() * 200, 1.0f, 8, 900, 70, 100, 150, 0); message(p->chr == 1 ? "DOWN TO YOUR UNDERSHIRT!" : "DOWN TO YOUR BOXERS!", 1.4f); }
    if (p->gear <= 0) { p->alive = 0; p->dead_t = 0; p->vy = -700; sfx("boom"); ink_burst(p->x, p->y - 60, 20, 300); }
}
static void kill_player(Player *p) { if (!p->alive) return; if (god_mode()) { reset_player(p, 0); return; } show_hit = 1; if (boss.active) boss_hit = 1; p->gear = 1; p->inv = 0; p->hidden = 0; hurt_player(p, p->x); }

/* ------------------------------------------------------------ init */
static Tex load_or_empty(const char *rel) { Tex t = tex_load(rel); return t; }
static Tex make_vignette(void) {
    int w = 320, h = 180; SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    Uint32 *px = (Uint32 *)s->pixels;
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        float dx = (x - w / 2.0f) / (w / 2.0f), dy = (y - h / 2.0f) / (h / 2.0f), d = sqrtf(dx * dx * 0.8f + dy * dy);
        float a = clampf((d - 0.55f) / 0.7f, 0, 1); a = a * a;
        px[y * (s->pitch / 4) + x] = SDL_MapRGBA(s->format, 20, 10, 4, (Uint8)(a * 235));
    }
    Tex t = tex_from_surface(s); SDL_FreeSurface(s); return t;
}
static Tex make_light_mask(void) {   /* black everywhere except a soft hole in the middle: lights-out flashlight */
    int w = 256; SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, w, 32, SDL_PIXELFORMAT_RGBA32);
    Uint32 *px = (Uint32 *)s->pixels;
    for (int y = 0; y < w; y++) for (int x = 0; x < w; x++) {
        float dx = (x - w / 2.0f) / (w / 2.0f), dy = (y - w / 2.0f) / (w / 2.0f), d = sqrtf(dx * dx + dy * dy);
        float a = clampf((d - 0.18f) / 0.5f, 0, 1);
        px[y * (s->pitch / 4) + x] = SDL_MapRGBA(s->format, 4, 2, 6, (Uint8)(a * 248));
    }
    Tex t = tex_from_surface(s); SDL_FreeSurface(s); return t;
}

void game_init(void) {
    srand(1234);
    font_load(&A.font, "font"); font_load(&A.small, "font_small"); font_set_small(&A.font, &A.small);
    const char *sets[NCHARS][3] = {{"hero", "hero_b", "hero_c"}, {"rosie", "rosie_b", "rosie_c"}};
    char rel[64];
    for (int c = 0; c < NCHARS; c++) for (int s = 0; s < 3; s++) for (int i = 0; i < HERO_FRAMES; i++) {
        snprintf(rel, sizeof rel, "chars/%s_%d.png", sets[c][s], i);
        Tex *t = &A.hero[c][s][i]; *t = load_or_empty(rel);
        if (!t->tex && s > 0) *t = A.hero[c][0][i];
        if (!t->tex && i > 0) *t = A.hero[c][s][0];   /* gear art not made yet: reuse full gear */
        if (!t->tex && c > 0) *t = A.hero[0][s][i];   /* character not drawn yet: borrow Joe */
    }
    for (int i = 0; i < 5; i++) {
        snprintf(rel, sizeof rel, "chars/usher_%d.png", i); A.usher[i] = load_or_empty(rel); if (!A.usher[i].tex && i) A.usher[i] = A.usher[i % 2];
        snprintf(rel, sizeof rel, "chars/projdog_%d.png", i); A.dog[i] = load_or_empty(rel); if (!A.dog[i].tex && i) A.dog[i] = A.dog[i % 2];
        if (i < 4) { snprintf(rel, sizeof rel, "chars/blob_%d.png", i); A.blob[i] = load_or_empty(rel); if (!A.blob[i].tex && i) A.blob[i] = A.blob[i % 2]; }
    }
    for (int i = 0; i < 12; i++) { snprintf(rel, sizeof rel, "chars/gore_%d.png", i); A.gore[i] = load_or_empty(rel); }
    for (int i = 0; i < 12; i++) { snprintf(rel, sizeof rel, "chars/gore2_%d.png", i); A.gore2[i] = load_or_empty(rel); if (!A.gore2[i].tex) A.gore2[i] = A.gore[i]; }
    for (int i = 0; i < 5; i++) {
        snprintf(rel, sizeof rel, "chars/goat_%d.png", i); A.goat[i] = load_or_empty(rel); if (!A.goat[i].tex && i) A.goat[i] = A.goat[0];
        snprintf(rel, sizeof rel, "chars/meat_%d.png", i); A.meat[i] = load_or_empty(rel); if (!A.meat[i].tex && i) A.meat[i] = A.meat[0];
        snprintf(rel, sizeof rel, "chars/slice_%d.png", i); A.slice[i] = load_or_empty(rel); if (!A.slice[i].tex && i) A.slice[i] = A.slice[0];
    }
    for (int i = 0; i < 5; i++) { snprintf(rel, sizeof rel, "chars/rat_%d.png", i); A.rat[i] = load_or_empty(rel); }
    A.cleaver = load_or_empty("chars/cleaver.png");
    A.bg2 = load_or_empty("bg/pizza.jpg");
    A.bg3 = load_or_empty("bg/reels.jpg");
    for (int i = 0; i < 5; i++) {
        snprintf(rel, sizeof rel, "chars/bomb_%d.png", i); A.bomb[i] = load_or_empty(rel); if (!A.bomb[i].tex && i) A.bomb[i] = A.bomb[0];
        snprintf(rel, sizeof rel, "chars/ghost_%d.png", i); A.ghost[i] = load_or_empty(rel); if (!A.ghost[i].tex && i) A.ghost[i] = A.ghost[0];
        snprintf(rel, sizeof rel, "chars/glove_%d.png", i); A.glove[i] = load_or_empty(rel); if (!A.glove[i].tex && i) A.glove[i] = A.glove[0];
    }
    for (int i = 0; i < 3; i++) { snprintf(rel, sizeof rel, "chars/splicer_%d.png", i); A.splicer[i] = load_or_empty(rel); }
    A.bg4 = load_or_empty("bg/inkworks.jpg");
    for (int i = 0; i < 5; i++) {
        snprintf(rel, sizeof rel, "chars/imp_%d.png", i); A.imp[i] = load_or_empty(rel); if (!A.imp[i].tex && i) A.imp[i] = A.imp[0];
        snprintf(rel, sizeof rel, "chars/worker_%d.png", i); A.worker[i] = load_or_empty(rel); if (!A.worker[i].tex && i) A.worker[i] = A.worker[0];
        snprintf(rel, sizeof rel, "chars/eel_%d.png", i); A.eel[i] = load_or_empty(rel); if (!A.eel[i].tex && i) A.eel[i] = A.eel[0];
    }
    for (int i = 0; i < 3; i++) { snprintf(rel, sizeof rel, "chars/press_%d.png", i); A.press[i] = load_or_empty(rel); }
    A.bg5 = load_or_empty("bg/lostfound.jpg");
    for (int i = 0; i < 5; i++) {
        snprintf(rel, sizeof rel, "chars/suitcase_%d.png", i); A.suitcase[i] = load_or_empty(rel); if (!A.suitcase[i].tex && i) A.suitcase[i] = A.suitcase[0];
        snprintf(rel, sizeof rel, "chars/soldier_%d.png", i); A.soldier[i] = load_or_empty(rel); if (!A.soldier[i].tex && i) A.soldier[i] = A.soldier[0];
        snprintf(rel, sizeof rel, "chars/umbrella_%d.png", i); A.umbrella[i] = load_or_empty(rel); if (!A.umbrella[i].tex && i) A.umbrella[i] = A.umbrella[0];
    }
    for (int i = 0; i < 3; i++) { snprintf(rel, sizeof rel, "chars/collector_%d.png", i); A.collector[i] = load_or_empty(rel); }
    A.bg6 = load_or_empty("bg/encore.jpg");
    for (int i = 0; i < 3; i++) { snprintf(rel, sizeof rel, "chars/unbound_%d.png", i); A.unbound[i] = load_or_empty(rel); }
    A.bg7 = load_or_empty("bg/mirrors.jpg");
    A.bg8 = load_or_empty("bg/rooftop.jpg");
    {   /* shows 6 and 7 */
        struct { const char *n; Tex *t; int k; } sets[] = {{"mannequin", A.mannequin, 5}, {"puff", A.puff, 5}, {"crow", A.crow, 5}, {"bulb", A.bulb, 5},
                                                           {"vanity", A.vanity, 3}, {"headliner", A.headliner, 3}};
        for (unsigned s = 0; s < sizeof sets / sizeof *sets; s++) for (int i = 0; i < sets[s].k; i++) {
            snprintf(rel, sizeof rel, "chars/%s_%d.png", sets[s].n, i); sets[s].t[i] = load_or_empty(rel);
            if (!sets[s].t[i].tex && i) sets[s].t[i] = sets[s].t[0]; }
    }
    for (int i = 0; i < 3; i++) { snprintf(rel, sizeof rel, "chars/inky_%d.png", i); A.inky[i] = load_or_empty(rel); }
    A.bg = load_or_empty("bg/theater.jpg");
    for (int i = 0; i < 4; i++) { snprintf(rel, sizeof rel, "ui/grain%d.png", i); A.grain[i] = load_or_empty(rel); }
    A.disc = make_circle_tex(64); A.vignette = make_vignette(); A.mask = make_light_mask();
    build_level();
    for (int i = 0; i < NUM_LEVELS; i++) { snprintf(rel, sizeof rel, "ui/posters/%d.jpg", i); posters[i] = load_or_empty(rel); }
    save_load_settings(); save_use_slot(0); audio_set_volume(save.music_vol, save.sfx_vol);
    ego_tex = load_or_empty("ui/ego.jpg"); keyart_tex = load_or_empty("ui/keyart.jpg"); iris_tex = make_iris();
    p1_char = getenv("MM_CHAR") ? atoi(getenv("MM_CHAR")) % NCHARS : save.runner % NCHARS;
    if (getenv("MM_DIFF")) save.difficulty = (unsigned char)(atoi(getenv("MM_DIFF")) % 3);   /* tests */
    scene = SC_LOGO; scene_t = 0; sfx("power"); sfx("ann_logo");   /* boot: studio logo -> press start -> save slots -> title */
}
void game_init_pad(void) {
    font_load(&padFont, "font"); font_load(&padSmall, "font_small"); padDisc = make_circle_tex(32);
}
int game_wants_quit(void) { return quit_req; }

void game_goto(const char *name) {
    if (!strcmp(name, "title")) { scene = SC_TITLE; return; }
    { static const struct { const char *n; int sc; } FRONT[] = {{"select", SC_SELECT}, {"options", SC_OPTIONS}, {"extras", SC_EXTRAS}, {"gallery", SC_GALLERY},
          {"jukebox", SC_JUKEBOX}, {"wardrobe", SC_WARDROBE}, {"credits", SC_CREDITS}};
      for (unsigned i = 0; i < sizeof FRONT / sizeof *FRONT; i++) if (!strcmp(name, FRONT[i].n)) { scene = FRONT[i].sc; scene_t = 0; opt_back = SC_TITLE; if (FRONT[i].sc == SC_JUKEBOX) build_tracks(); if (FRONT[i].sc == SC_CREDITS) credits_then = 2; return; }
      if (!strcmp(name, "intro")) { start_intro(0); return; }
      if (!strcmp(name, "logo")) { scene = SC_LOGO; scene_t = 0; return; }
      if (!strcmp(name, "press")) { to_press(); return; }
      if (!strcmp(name, "slots")) { open_slots(); return; }
      if (!strcmp(name, "demo")) { start_demo(); return; }
      if (!strncmp(name, "load", 4)) { rush_on = 0; queue_show(atoi(name + 4) > 0 ? atoi(name + 4) - 1 : 0, 0); return; }
      if (!strcmp(name, "rush")) { rush_on = 1; rush_time = 0; queue_show(0, 0); return; } }
    if (!strncmp(name, "level2", 6) || !strcmp(name, "boss2") || !strcmp(name, "kitchen") || !strcmp(name, "dock")) {
        level_no = 1; start_level();
        if (!strcmp(name, "boss2")) { cur_check = ncheck - 1; reset_player(&pl[0], 0); }
        if (!strcmp(name, "kitchen")) { cur_check = 3; reset_player(&pl[0], 0); }
        if (!strcmp(name, "dock")) { cur_check = 4; reset_player(&pl[0], 0); }
        return;
    }
    if (!strcmp(name, "level7") || !strcmp(name, "boss7") || !strcmp(name, "mirrors") || !strcmp(name, "dressing")) {
        level_no = L_MIRROR; start_level();
        if (!strcmp(name, "boss7")) { cur_check = ncheck - 1; reset_player(&pl[0], 0); }
        if (!strcmp(name, "mirrors")) { cur_check = 3; reset_player(&pl[0], 0); }
        if (!strcmp(name, "dressing")) { cur_check = 4; reset_player(&pl[0], 0); }
        return;
    }
    if (!strcmp(name, "level8") || !strcmp(name, "boss8") || !strcmp(name, "neon") || !strcmp(name, "rods")) {
        level_no = L_ROOF; start_level();
        if (!strcmp(name, "boss8")) { cur_check = ncheck - 1; reset_player(&pl[0], 0); }
        if (!strcmp(name, "neon")) { cur_check = 3; reset_player(&pl[0], 0); }
        if (!strcmp(name, "rods")) { cur_check = 4; reset_player(&pl[0], 0); }
        return;
    }
    if (!strcmp(name, "level6") || !strcmp(name, "boss6") || !strcmp(name, "pit") || !strcmp(name, "loft") || !strcmp(name, "curtain") || !strcmp(name, "ending") || !strcmp(name, "loop2")) {
        level_no = L_ENCORE; if (!strcmp(name, "loop2")) loop_no = 1; start_level();
        if (!strcmp(name, "boss6") || !strcmp(name, "ending") || !strcmp(name, "loop2")) { cur_check = ncheck - 1; reset_player(&pl[0], 0); }
        if (!strcmp(name, "pit")) { cur_check = 2; reset_player(&pl[0], 0); }
        if (!strcmp(name, "loft")) { cur_check = 3; reset_player(&pl[0], 0); }
        if (!strcmp(name, "curtain")) { cur_check = 4; reset_player(&pl[0], 0); }
        if (!strcmp(name, "ending")) { scene = SC_END; scene_t = 0; music_play("mm_ending", 0); }
        return;
    }
    if (!strcmp(name, "level5") || !strcmp(name, "boss5") || !strcmp(name, "toys") || !strcmp(name, "luggage") || !strcmp(name, "dolls")) {
        level_no = 4; start_level();
        if (!strcmp(name, "boss5")) { cur_check = ncheck - 1; reset_player(&pl[0], 0); }
        if (!strcmp(name, "toys")) { cur_check = 2; reset_player(&pl[0], 0); }
        if (!strcmp(name, "luggage")) { cur_check = 3; reset_player(&pl[0], 0); }
        if (!strcmp(name, "dolls")) { cur_check = 4; reset_player(&pl[0], 0); }
        return;
    }
    if (!strcmp(name, "level4") || !strcmp(name, "boss4") || !strcmp(name, "vats") || !strcmp(name, "pipes") || !strcmp(name, "furnace")) {
        level_no = 3; start_level();
        if (!strcmp(name, "boss4")) { cur_check = ncheck - 1; reset_player(&pl[0], 0); }
        if (!strcmp(name, "vats")) { cur_check = 2; reset_player(&pl[0], 0); }
        if (!strcmp(name, "pipes")) { cur_check = 3; reset_player(&pl[0], 0); }
        if (!strcmp(name, "furnace")) { cur_check = 4; reset_player(&pl[0], 0); }
        return;
    }
    if (!strcmp(name, "level3") || !strcmp(name, "boss3") || !strcmp(name, "vault") || !strcmp(name, "cutting") || !strcmp(name, "spook")) {
        level_no = 2; start_level();
        if (!strcmp(name, "boss3")) { cur_check = ncheck - 1; reset_player(&pl[0], 0); }
        if (!strcmp(name, "spook")) { cur_check = 2; reset_player(&pl[0], 0); }
        if (!strcmp(name, "vault")) { cur_check = 3; reset_player(&pl[0], 0); }
        if (!strcmp(name, "cutting")) { cur_check = 4; reset_player(&pl[0], 0); }
        return;
    }
    start_level();
    if (!strcmp(name, "boss")) { cur_check = ncheck - 1; reset_player(&pl[0], 0); }
    if (!strcmp(name, "fiend")) { cur_check = 3; reset_player(&pl[0], 0); }
    if (!strcmp(name, "booth")) { cur_check = 4; reset_player(&pl[0], 0); }
    if (!strcmp(name, "dark")) { power = 0; }
    if (!strcmp(name, "coop")) { mode = 1; start_level(); }
}

/* ------------------------------------------------------------ player update */
static void fire(Player *p) {
    float dx = p->aim_dx, dy = p->aim_dy;
    float len = sqrtf(dx * dx + dy * dy); if (len < 0.1f) { dx = p->face; dy = 0; len = 1; }
    dx /= len; dy /= len;
    float mx = p->x + p->face * 40, my = p->y - 78;
    if (p->aim_dy < 0 && p->aim_dx != 0) { mx = p->x + p->face * 42; my = p->y - 122; }
    else if (p->aim_dy < 0) { mx = p->x + p->face * 18; my = p->y - 152; }
    else if (p->aim_dy > 0) { mx = p->x + p->aim_dx * 20; my = p->y - 30; }
    else if (p->crouch) { my = p->y - 48; }
    p->shoot_t = 0.15f;
    for (int k = 0; k < (p->weapon == W_TORCH ? 1 : 3); k++) part(mx + dx * 10, my + dy * 10, dx * (200 + frand() * 300) + (frand() - 0.5f) * 120, dy * (200 + frand() * 300) + (frand() - 0.5f) * 120, 0.12f, 4, 0, 255, 230, 150, 1);
    if (p->weapon == W_NAIL || p->weapon == W_RIVET) {   /* spent casing flips out the side */
        Part *c = part_k(PK_CASING, p->x + p->face * 10, my + 6, -p->face * (120 + frand() * 120), -260 - frand() * 160, 2.5f, 3, 1600, 220, 180, 80, 0);
        if (c) c->vang = 1400;
    }
    if (p->weapon == W_RIVET) smoke(mx, my, 1, 120);
    switch (p->weapon) {
    case W_NAIL: proj(P_NAIL, mx, my, dx * 1150, dy * 1150, 0, 1); p->cool = 0.11f; sfx("throw"); break;
    case W_RIVET:
        for (int k = -1; k <= 1; k++) { float a = atan2f(dy, dx) + k * 0.2f; Proj *q = proj(P_RIVET, mx, my, cosf(a) * 950, sinf(a) * 950, 0, 1); if (q) q->r = 9; }
        p->cool = 0.24f; sfx("rivet"); break;
    case W_TORCH: {
        float a = atan2f(dy, dx) + (frand() - 0.5f) * 0.25f;
        Proj *q = proj(P_FLAME, mx, my, cosf(a) * 720, sinf(a) * 720, 0, 1); if (q) { q->life = 0.24f; q->r = 16; }
        p->cool = 0.05f; if (frame_no % 8 == 0) sfx("sizzle"); break; }
    case W_PLUNGER: {
        Proj *q = proj(P_PLUNGER, mx, my, dx * 520 + p->vx * 0.3f, dy * 520 - 380, 0, 2); if (q) { q->grav = 1500; q->r = 14; }
        p->cool = 0.45f; sfx("boing"); break; }
    }
}

static void update_player(Player *p, const Input *in) {
    if (!p->active) return;
    if (!p->alive) {
        p->dead_t += DT; p->vy += 1800 * DT; p->y += p->vy * DT;
        if (p->dead_t > 1.6f) {
            p->lives--; show_deaths++;
            if (p->lives > 0) { reset_player(p, 0); p->weapon = W_NAIL; }
            else { p->active = 0; p->alive = 0; }
        }
        return;
    }
    if (p->inv > 0) p->inv -= DT;
    if (p->cool > 0) p->cool -= DT;
    if (p->shoot_t > 0) p->shoot_t -= DT;
    unsigned h = in->held, pr_ = in->pressed;
    int L = (h & B_LEFT) != 0, R = (h & B_RIGHT) != 0, U = (h & B_UP) != 0, D = (h & B_DOWN) != 0;

    /* hiding in a prop closet */
    int ctx = (int)(p->x / TILE);
    if (p->hidden) {
        if ((pr_ & (B_UP | B_JUMP | B_DOWN))) { p->hidden = 0; p->inv = 0.4f; sfx("closet"); }
        return;
    }
    if ((pr_ & B_UP) && p->onground && map[12][ctx] == 'C') {
        p->hidden = 1; p->closet_tx = ctx; p->x = ctx * TILE + TILE / 2; p->vx = 0; sfx("closet"); award(19);
        message("HIDING... (UP TO COME OUT)", 1.2f); return;
    }
    /* flip a breaker */
    if ((pr_ & B_UP) && p->onground) for (int i = 0; i < nbreak; i++) if (!breakers[i].used && abs(breakers[i].tx - ctx) <= 1) {
        breakers[i].used = 1; power = clampf(power + 40, 0, 100); sfx("power");
        if (!demo_on && save.breakers_flipped < 255) { save.breakers_flipped++; if (save.breakers_flipped >= 25) award(12); } vo(p->chr, "lights"); message("BREAKER ON: +40% POWER", 1.6f);
        spark_burst(breakers[i].tx * TILE + 24, 11 * TILE + 20, 20, 255, 230, 120);
    }

    int lock = (h & B_LOCK) != 0;
    p->crouch = D && p->onground && !L && !R && !lock;
    float target = 0;
    if (!lock && !p->crouch) { if (L) target = -340; if (R) target = 340; }
    if (L) p->face = -1; if (R) p->face = 1;
    float accel = p->onground ? 3200 : 2000;
    if (p->vx < target) p->vx = fminf(target, p->vx + accel * DT); else if (p->vx > target) p->vx = fmaxf(target, p->vx - accel * DT);

    /* aim: Contra style */
    p->aim_dx = p->face; p->aim_dy = 0;
    if (U) { p->aim_dy = -1; p->aim_dx = (L || R) ? p->face : 0; }
    else if (D && !p->onground) { p->aim_dy = 1; p->aim_dx = (L || R) ? p->face : 0; }
    else if (D && lock) { p->aim_dy = 1; p->aim_dx = (L || R) ? p->face : 0; }

    int drop = D && (pr_ & B_JUMP) && p->onground && oneway_at(ctx, (int)(p->y / TILE));
    if ((pr_ & B_JUMP) && p->onground && !drop) { p->vy = -920; p->onground = 0; sfx("jump"); p->jump_t = 0.16f; smoke(p->x, p->y - 4, 2, 170); }
    if (!(h & B_JUMP) && p->vy < -300) p->vy += 3600 * DT;    /* short hop when the button is let go */
    p->vy += 2500 * DT; if (p->vy > 1300) p->vy = 1300;
    float w = 46, hh = p->crouch ? 80 : 124;
    float vy_before = p->vy;
    int r = move_box(&p->x, &p->y, &p->vx, &p->vy, w, hh, drop);
    p->onground = (r & 1) != 0;
    /* cartoon timing: squash on landing, a puff of dust, a turn that flips through the middle */
    if (p->onground && !p->was_ground && vy_before > 450) { p->land_t = 0.14f; smoke(p->x, p->y - 4, vy_before > 900 ? 5 : 3, 175); sfx(land_sfx()); }
    p->was_ground = p->onground;
    if (p->face != p->prev_face) { if (p->onground) p->turn_t = 0.1f; p->prev_face = p->face; }
    if (p->land_t > 0) p->land_t -= DT;
    if (p->jump_t > 0) p->jump_t -= DT;
    if (p->turn_t > 0) p->turn_t -= DT;
    if (p->hurt_t > 0) p->hurt_t -= DT;
    if (p->onground && fabsf(p->vx) > 200) { p->step_t -= DT; if (p->step_t <= 0) { p->step_t = 0.26f; part_k(PK_SMOKE, p->x - p->face * 14, p->y - 4, -p->face * 40, -30, 0.45f, 7, -10, 170, 160, 150, 0); } }
    p->idle_t = (p->onground && fabsf(p->vx) < 20 && !(h & (B_SHOOT | B_LOCK))) ? p->idle_t + DT : 0;
    if (p->onground) conveyor_carry(&p->x, &p->y, w, hh);
    if (arena_locked) p->x = clampf(p->x, ARENA_X + 30, ARENA_X + SCREEN_W - 30);
    p->x = clampf(p->x, 30, LW * TILE - 30);
    if (p->vx != 0 && p->onground) p->anim += DT * fabsf(p->vx) / 340; else p->anim = 0;
    p->bob += DT;

    if ((h & B_SHOOT) && p->cool <= 0) fire(p);

    /* fell into the ink */
    if (p->onground) {   /* remember the last solid, solid-looking ground (Matinee mode puts you back here after a fall) */
        int tx = (int)(p->x / TILE), ty = (int)((p->y + 4) / TILE);
        if (ty >= 0 && ty < LH && tx > 0 && tx < LW - 1) { char c = map[ty][tx], l = map[ty][tx - 1], r2 = map[ty][tx + 1];
            if ((c == '#' || c == '=') && (l == '#' || l == '=') && (r2 == '#' || r2 == '=')) { p->safe_x = p->x; p->safe_y = p->y; } }
    }
    if (p->y > FLOOR_Y + 30 || p->y > LH * TILE + 100) { sfx("splash"); ink_burst(p->x, FLOOR_Y, 25, 300);
        if (difficulty() == 0 && p->gear > 1 && !god_mode()) {   /* Matinee: a fall costs one piece of gear, not a life */
            p->gear--; p->inv = 1.6f; p->x = p->safe_x; p->y = p->safe_y - 2; p->vx = p->vy = 0; shake = 6; sfx("hurt");
            rumble_pad(p->pad, 0.6f, 220); message("SPLASH! BACK ON SOLID GROUND", 1.2f); }
        else kill_player(p); }
    /* checkpoints */
    for (int i = cur_check + 1; i < ncheck; i++) if (p->x > checkpoints[i] * TILE) {
        cur_check = i; sfx("checkpoint"); message("GHOST LIGHT LIT: CHECKPOINT", 1.5f);
        spark_burst(checkpoints[i] * TILE + 24, 12 * TILE - 40, 30, 255, 220, 140);
    }
    /* pickups */
    for (int i = 0; i < 8; i++) if (pick[i].alive && overlap(p->x, p->y, w, hh, pick[i].x, pick[i].y, 50, 50)) {
        pick[i].alive = 0; sfx("pickup");
        if (pick[i].kind == 10) { if (p->gear < 3) p->gear++; message(p->gear == 3 ? "FULL GEAR!" : p->chr == 1 ? "COVERALLS BACK ON!" : "OVERALLS BACK ON!", 1.4f); score += 500; }
        else { int k = ((int)(pick[i].t / 0.6f)) % NWEAP; p->weapon = k; message(WEAP_NAME[k], 1.4f); score += 200; }
    }
}

/* ------------------------------------------------------------ enemies */
static Player *nearest_player(float x, float y, float *dist) {
    Player *best = NULL; float bd = 1e9f;
    for (int i = 0; i < 2; i++) { Player *p = &pl[i]; if (!p->active || !p->alive || p->hidden) continue;
        float d = fabsf(p->x - x) + fabsf(p->y - y) * 0.5f; if (d < bd) { bd = d; best = p; } }
    if (dist) *dist = bd; return best;
}
static void enemy_box(Enemy *e, float *w, float *h) {
    switch (e->type) {
    case E_USHER: *w = 60; *h = 150; break;
    case E_DOG: *w = 92; *h = 112; break;
    case E_BLOB: *w = 64; *h = 60; break;
    case E_MINIBLOB: *w = 40; *h = 38; break;
    case E_GOAT: *w = 64; *h = 150; break;
    case E_MEATBALL: *w = 72; *h = 70; break;
    case E_SLICE: *w = 76; *h = 54; break;
    case E_BOMB: *w = 64; *h = 76; break;
    case E_GHOST: *w = 76; *h = 104; break;
    case E_GLOVE: *w = 90; *h = 66; break;
    case E_IMP: *w = 60; *h = 84; break;
    case E_SUITCASE: *w = 84; *h = 74; break;
    case E_SHADOW: *w = 44; *h = 140; break;
    case E_SOLDIER: *w = 56; *h = 140; break;
    case E_UMBRELLA: *w = 84; *h = 56; break;
    case E_MANNEQUIN: *w = 60; *h = 150; break;
    case E_PUFF: *w = 64; *h = 54; break;
    case E_MIRROR: *w = 44; *h = 140; break;
    case E_CROW: *w = 74; *h = 52; break;
    case E_BULB: *w = 56; *h = 100; break;
    case E_WORKER: *w = 70; *h = 140; break;
    case E_EEL: *w = 60; *h = (e->state == 1 ? 80 : e->state == 2 ? 150 : 20); break;
    default: *w = 56; *h = 48; break;
    }
}
/* film ghosts fade out of the picture for a while: you can't hit them (or be hit) while they're faded */
static int ghost_faded(Enemy *e) { return e->type == E_GHOST && e->state == 0 && fmodf(e->t + e->land, 4.0f) > 2.6f; }
static void enemy_damage(Enemy *e, int dmg);
static void bomb_blast(float x, float y) {   /* cartoon bombs: a big black-and-white KA-BOOM that hurts everyone nearby */
    sfx("boom"); if (shake < 12) shake = 12; hitstop = 3;
    ring(x, y, 180, 255, 255, 255); spark_burst(x, y, 30, 255, 240, 200); smoke(x, y, 10, 40); embers(x, y, 8);
    for (int i = 0; i < 14; i++) part_k(PK_DEBRIS, x, y, (frand() - 0.5f) * 800, -200 - frand() * 600, 2.0f, 6 + frand() * 8, 1600, 24, 22, 24, 0);
    for (int i = 0; i < 18; i++) { float a = frand() * 6.283f, v = 300 + frand() * 400; part(x, y, cosf(a) * v, sinf(a) * v, 0.35f, 10, 0, 255, 255, 255, 1); }
    for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && fabsf(p->x - x) < 150 && fabsf((p->y - 60) - y) < 150) hurt_player(p, x); }
    for (int j = 0; j < nen; j++) { Enemy *o = &en[j]; if (o->alive && o->type < E_CRATE && fabsf(o->x - x) < 150 && fabsf(o->y - 40 - y) < 150) enemy_damage(o, 3); }
}
static Tex *enemy_tex(Enemy *e) {
    int walk = ((int)(e->t * 7)) % 4;
    switch (e->type) {
    case E_USHER: return &A.usher[e->state == 1 ? 4 : (fabsf(e->vx) > 5 ? walk : 0)];
    case E_DOG: return &A.dog[e->state >= 1 ? 4 : walk];
    case E_BLOB: case E_MINIBLOB: return &A.blob[!e->onground ? 2 : e->land > 0 ? 3 : e->timer < 0.25f ? 1 : 0];
    case E_GOAT: return &A.goat[e->state == 1 ? 4 : (fabsf(e->vx) > 5 ? walk : 0)];
    case E_MEATBALL: return &A.meat[e->state == 1 ? 4 : ((int)(e->t * 10)) % 4];
    case E_SLICE: return &A.slice[e->state == 1 ? 4 : ((int)(e->t * 10)) % 4];
    case E_BOMB: return &A.bomb[e->state == 1 ? 4 : !e->onground ? 2 : e->land > 0 ? 3 : e->timer < 0.2f ? 1 : 0];
    case E_GHOST: return &A.ghost[e->state == 1 ? 4 : ghost_faded(e) ? 3 : ((int)(e->t * 3)) % 2];
    case E_GLOVE: return &A.glove[e->state == 0 ? ((int)(e->t * 8)) % 2 : e->state == 1 ? 2 : e->state == 2 ? 3 : 4];
    case E_SHADOW: return &A.hero[pl[0].chr][0][!e->onground ? 5 : fabsf(e->vx) > 20 ? 1 + ((int)(e->t * 8)) % 4 : 0];
    case E_SUITCASE: return &A.suitcase[e->flash > 3 ? 4 : !e->onground ? 2 : e->land > 0 ? 3 : e->timer < 0.25f ? 1 : 0];
    case E_SOLDIER: return &A.soldier[e->flash > 3 ? 4 : e->state == 1 ? (e->timer > 0.15f ? 2 : 3) : (fabsf(e->vx) > 5 ? ((int)(e->t * 6)) % 2 : 0)];
    case E_UMBRELLA: return &A.umbrella[e->flash > 3 ? 4 : e->state == 1 ? 3 : ((int)(e->t * 8)) % 3];
    case E_MANNEQUIN: return &A.mannequin[e->flash > 3 ? 4 : e->state == 2 ? 3 : e->state == 1 ? 1 + ((int)(e->t * 6)) % 2 : 0];
    case E_PUFF: return &A.puff[e->flash > 3 ? 4 : e->state == 1 ? 2 : ((int)(e->t * 8)) % 2];
    case E_CROW: return &A.crow[e->flash > 3 ? 4 : e->state == 1 ? 2 : ((int)(e->t * 7)) % 2];
    case E_BULB: return &A.bulb[e->flash > 3 ? 4 : e->state == 1 ? 2 : e->state == 2 ? 3 : fabsf(e->vx) > 5 ? ((int)(e->t * 6)) % 2 : 0];
    case E_MIRROR: { Player *src = &pl[0]; int set = src->gear >= 3 ? 0 : src->gear == 2 ? 1 : 2;
        return &A.hero[src->chr][set][!e->onground ? 5 : fabsf(e->vx) > 20 ? 1 + ((int)(e->t * 8)) % 4 : 0]; }
    case E_IMP: return &A.imp[e->flash ? 4 : e->state == 1 ? (e->timer > 0.2f ? 2 : 3) : (fabsf(e->vx) > 5 ? ((int)(e->t * 7)) % 2 : 0)];
    case E_WORKER: return &A.worker[e->flash > 3 ? 4 : e->state == 1 ? 2 : e->state == 2 ? 3 : (fabsf(e->vx) > 5 ? ((int)(e->t * 7)) % 2 : 0)];
    case E_EEL: return &A.eel[e->state == 1 ? (e->timer > 0.2f ? 0 : 1) : e->state == 2 ? (e->timer < 0.6f && e->timer > 0.25f ? 3 : 2) : 4];
    }
    return NULL;
}
static void gore_enemy(Enemy *e) {
    float cx = e->x, cy = e->y - 60, k = e->type == E_MINIBLOB ? 0.6f : 1.0f;
    Tex *t = enemy_tex(e);
    switch (e->type) {
    case E_USHER:   /* porcelain doll: head cracks open, arms fly, oil-blood everywhere */
        sfx("shatter");
        shatter(t, e->x, e->y, e->face > 0, 3, 4, 420, 1);
        gore_part(2, cx, cy - 60, 520, 0); gore_part(2, cx, cy - 60, 480, 0); gore_part(3, cx, cy - 70, 600, 1);
        gore_part(3, cx, cy - 70, 560, 1); gore_part(4, cx, cy - 20, 500, 1); gore_part(4, cx, cy - 20, 460, 1);
        gore_part(10, cx, cy, 500, 0); gore_part(11, cx, cy, 380, 1); gore_part(0, cx, cy, 450, 0);
        blood_spurt(cx, cy - 40, -e->face, 40, 620); blood_spurt(cx, cy - 40, e->face, 20, 500);
        break;
    case E_DOG:     /* projector dog: blows apart into brass, lens glass and springs */
        shatter(t, e->x, e->y, e->face > 0, 4, 3, 480, 1);
        gore_part(6, cx, cy, 600, 0); gore_part(0, cx, cy, 560, 0); gore_part(0, cx, cy, 520, 0); gore_part(1, cx, cy, 540, 0);
        gore_part(7, cx, cy, 500, 0); gore_part(5, cx, cy, 460, 1); gore_part(3, cx, cy - 20, 580, 1);
        blood_spurt(cx, cy, -e->face, 26, 560); spark_burst(cx, cy, 24, 255, 210, 110); smoke(cx, cy, 8, 70);
        ring(cx, cy, 110, 255, 220, 150); sfx("boom"); sfx("clank");
        break;
    case E_GOAT:    /* party goat: horns, accordion keys and party hat everywhere */
        sfx("clank"); shatter(t, e->x, e->y, e->face > 0, 3, 4, 420, 1);
        gore_part2(0, cx, cy - 70, 560, 0); gore_part2(0, cx, cy - 70, 520, 0); gore_part2(1, cx, cy, 500, 0); gore_part2(1, cx, cy, 460, 0);
        gore_part2(2, cx, cy - 80, 600, 0); gore_part2(3, cx, cy - 60, 580, 1); gore_part2(10, cx, cy, 480, 0);
        blood_spurt(cx, cy - 40, -e->face, 36, 600);
        for (int i = 0; i < 24; i++) part(cx, cy - 40, (frand() - 0.5f) * 700, -200 - frand() * 500, 1.2f, 4, 900, 120 + rand() % 136, 80 + rand() % 176, 80 + rand() % 176, 0);   /* confetti */
        break;
    case E_MEATBALL: /* meatball: chunks and sauce */
        sfx("splat"); shatter(t, e->x, e->y, e->face > 0, 3, 3, 380, 1);
        for (int k = 0; k < 4; k++) gore_part2(4, cx, cy, 520, 1);
        gore_part2(3, cx, cy, 560, 1);
        for (int i = 0; i < 30; i++) { float a = frand() * 6.283f, v = 200 + frand() * 450;
            part_k(PK_BLOOD, cx, cy, cosf(a) * v, sinf(a) * v - 250, 1.4f, 4 + frand() * 7, 1500, 170 + rand() % 40, 20, 14, 0); }
        break;
    case E_SLICE:   /* pizza bat: pepperoni, wings, cheese */
        sfx("splat"); shatter(t, e->x, e->y, e->face > 0, 3, 2, 360, 0);
        gore_part2(6, cx, cy, 500, 0); gore_part2(6, cx, cy, 460, 0); gore_part2(7, cx, cy, 520, 0); gore_part2(7, cx, cy, 480, 0); gore_part2(5, cx, cy, 440, 0);
        blood_spurt(cx, cy, -e->face, 14, 450);
        break;
    case E_BOMB:    /* blows up for real */
        if (t) shatter(t, e->x, e->y, e->face > 0, 3, 3, 520, 0);
        bomb_blast(cx, e->y - 40);
        break;
    case E_GHOST:   /* tears like old film: grey scraps and a puff of ink */
        sfx("poof"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 3, 380, 0);
        for (int i = 0; i < 24; i++) part(cx, cy, (frand() - 0.5f) * 600, -150 - frand() * 400, 1.0f, 5, 700, 200, 200, 196, 0);
        for (int i = 0; i < 20; i++) { float a = frand() * 6.283f, v = 200 + frand() * 400;
            part_k(PK_BLOOD, cx, cy, cosf(a) * v, sinf(a) * v - 200, 1.4f, 4 + frand() * 6, 1500, 12, 8, 12, 0); }
        break;
    case E_GLOVE:   /* the eye in the palm pops; red is the only colour left in this cartoon */
        sfx("splat"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 3, 420, 1);
        gore_part(11, cx, cy, 480, 1); gore_part(11, cx, cy, 440, 1); gore_part(3, cx, cy, 520, 1);
        blood_spurt(cx, cy, -e->face, 34, 600); blood_spurt(cx, cy, e->face, 18, 480);
        break;
    case E_SHADOW:  /* an ink understudy: bursts back into ink */
        sfx("splat"); sfx("fiendwarn");
        for (int i = 0; i < 44; i++) { float a = frand() * 6.283f, v = 200 + frand() * 500;
            part_k(PK_BLOOD, cx, cy - 20, cosf(a) * v, sinf(a) * v - 250, 1.4f, 4 + frand() * 9, 1500, 12, 8, 12, 0); }
        ring(cx, cy - 20, 110, 40, 30, 40); smoke(cx, cy - 20, 6, 20);
        break;
    case E_SUITCASE: /* bursts open: clothes, papers, a toothy lid */
        sfx("splat"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 3, 440, 1);
        for (int i = 0; i < 16; i++) part_k(PK_DEBRIS, cx, cy, (frand() - 0.5f) * 700, -200 - frand() * 500, 2.0f, 6 + frand() * 8, 1500, 200 + rand() % 50, 190 + rand() % 50, 160, 0);
        blood_spurt(cx, cy, -e->face, 30, 560); gore_part(3, cx, cy, 520, 1);
        break;
    case E_SOLDIER: /* splinters: wooden limbs, a bearskin hat, springs */
        sfx("clank"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 4, 460, 1);
        for (int i = 0; i < 12; i++) part_k(PK_DEBRIS, cx, cy - 20, (frand() - 0.5f) * 600, -200 - frand() * 500, 2.0f, 6 + frand() * 8, 1500, 150, 100, 55, 0);
        gore_part(5, cx, cy, 520, 0); blood_spurt(cx, cy - 40, -e->face, 22, 520);
        break;
    case E_UMBRELLA: /* shredded canopy and a snapped handle */
        sfx("splat"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 2, 380, 1);
        for (int i = 0; i < 14; i++) part(cx, cy, (frand() - 0.5f) * 600, -150 - frand() * 350, 1.2f, 6, 800, 20, 18, 22, 0);
        blood_spurt(cx, cy, -e->face, 16, 460);
        break;
    case E_IMP:     /* the inkwell shatters: glass and a big ink splash */
        sfx("shatter"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 3, 420, 0);
        for (int i = 0; i < 36; i++) { float a = frand() * 6.283f, v = 200 + frand() * 450;
            part_k(PK_BLOOD, cx, cy, cosf(a) * v, sinf(a) * v - 250, 1.4f, 4 + frand() * 8, 1500, 12, 8, 12, 0); }
        for (int i = 0; i < 10; i++) part_k(PK_DEBRIS, cx, cy, (frand() - 0.5f) * 600, -200 - frand() * 400, 1.5f, 4 + frand() * 5, 1500, 200, 220, 230, 0);
        break;
    case E_WORKER:  /* tin worker bursts: springs, rivets, sparks and oil */
        sfx("clank"); sfx("boom"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 4, 480, 1);
        gore_part(6, cx, cy, 600, 0); gore_part(5, cx, cy, 560, 0); gore_part(5, cx, cy - 40, 520, 0); gore_part(0, cx, cy, 500, 0); gore_part(1, cx, cy, 520, 0);
        spark_burst(cx, cy - 30, 26, 255, 210, 110); smoke(cx, cy - 30, 6, 80); blood_spurt(cx, cy - 40, -e->face, 24, 520);
        ring(cx, cy - 30, 100, 255, 210, 150);
        break;
    case E_MANNEQUIN:   /* splinters: wooden limbs, the wire skirt, stuffing */
        sfx("clank"); if (t) shatter(t, e->x, e->y, e->face > 0, 3, 4, 460, 0);
        for (int i = 0; i < 14; i++) part_k(PK_DEBRIS, cx, cy, (frand() - 0.5f) * 600, -200 - frand() * 400, 1.6f, 5 + frand() * 6, 1600, 220, 200, 160, 0);
        for (int i = 0; i < 16; i++) part(cx, cy, (frand() - 0.5f) * 500, -150 - frand() * 300, 1.0f, 6, 600, 240, 235, 220, 0);
        blood_spurt(cx, cy - 40, -e->face, 18, 480);
        break;
    case E_PUFF:        /* bursts in a cloud of pink powder */
        sfx("poof"); if (t) shatter(t, e->x, e->y, e->face > 0, 2, 2, 300, 0);
        for (int i = 0; i < 40; i++) { float a = frand() * 6.283f, v = 60 + frand() * 260; part(cx, e->y - 20, cosf(a) * v, sinf(a) * v - 60, 1.4f + frand(), 10 + frand() * 12, -40, 255, 190 + rand() % 40, 210, 0); }
        break;
    case E_MIRROR:      /* the reflection shatters like glass */
        sfx("shatter");
        for (int i = 0; i < 30; i++) part_k(PK_DEBRIS, cx, cy - 20, (frand() - 0.5f) * 800, -200 - frand() * 500, 1.6f, 4 + frand() * 7, 1600, 190 + rand() % 50, 215, 235, 0);
        spark_burst(cx, cy - 20, 18, 230, 245, 255); ring(cx, cy - 20, 90, 220, 240, 255);
        break;
    case E_CROW:        /* feathers everywhere */
        sfx("splat"); if (t) shatter(t, e->x, e->y, e->face > 0, 2, 2, 380, 1);
        for (int i = 0; i < 22; i++) part_k(PK_DEBRIS, cx, e->y - 20, (frand() - 0.5f) * 500, -150 - frand() * 300, 2.2f, 6 + frand() * 6, 300, 20, 18, 26, 0);
        blood_spurt(cx, e->y - 20, -e->face, 16, 460);
        break;
    case E_BULB:        /* pops: glass, a flash and sparks */
        sfx("shatter"); sfx("beam"); if (t) shatter(t, e->x, e->y, e->face > 0, 2, 3, 420, 0);
        spark_burst(cx, cy - 20, 30, 255, 240, 140); ring(cx, cy - 20, 110, 255, 240, 160);
        for (int i = 0; i < 12; i++) part_k(PK_DEBRIS, cx, cy - 20, (frand() - 0.5f) * 600, -200 - frand() * 400, 1.5f, 4 + frand() * 5, 1600, 240, 240, 220, 0);
        break;
    case E_EEL:     /* the eel bursts in a spray of ink and red */
        sfx("splat"); if (t) shatter(t, e->x, e->y, e->face > 0, 2, 4, 400, 1);
        blood_spurt(cx, e->y - 90, -e->face, 30, 600);
        for (int i = 0; i < 30; i++) { float a = frand() * 6.283f, v = 200 + frand() * 450;
            part_k(PK_BLOOD, cx, e->y - 80, cosf(a) * v, sinf(a) * v - 250, 1.4f, 4 + frand() * 7, 1500, 12, 8, 12, 0); }
        break;
    default:        /* ink blob: bursts into a big ink splash that paints the room */
        sfx("splat");
        shatter(t, e->x, e->y, e->face > 0, 2, 2, 300 * k, 0);
        for (int i = 0; i < (int)(30 * k); i++) { float a = frand() * 6.283f, v = 200 + frand() * 450;
            part_k(PK_BLOOD, cx, e->y - 30 * k, cosf(a) * v, sinf(a) * v - 250, 1.4f, 4 + frand() * 8 * k, 1500, 12, 8, 12, 0); }
        break;
    }
    hitstop = e->type == E_MINIBLOB ? 1 : 3;
}
static void enemy_damage(Enemy *e, int dmg) {
    if (e->marked > 0) dmg *= 2;
    e->hp -= dmg; e->flash = 6; sfx("hit");
    if (e->type != E_CRATE && e->type != E_HATCRATE) {   /* every hit draws a little oil-blood */
        float w, h; enemy_box(e, &w, &h);
        blood_spurt(e->x, e->y - h * 0.6f, e->face > 0 ? -1 : 1, 6, 380);
        e->vx += (e->face > 0 ? -1 : 1) * 60;
    }
    if (e->hp > 0) return;
    e->alive = 0; shake = 6;
    float cx = e->x, cy = e->y - 40;
    if (e->type == E_CRATE || e->type == E_HATCRATE) {
        for (int i = 0; i < 8; i++) if (!pick[i].alive) { pick[i] = (Pickup){1, e->type == E_HATCRATE ? 10 : 0, cx, e->y - 30, -400, frand() * 2}; break; }
        spark_burst(cx, cy, 16, 200, 150, 90); sfx("poof");
        for (int i = 0; i < 8; i++) part_k(PK_DEBRIS, cx, cy, (frand() - 0.5f) * 500, -200 - frand() * 400, 2.5f, 10 + frand() * 8, 1600, 150, 100, 55, 0);
        return;
    }
    score += e->type == E_DOG ? 400 : e->type == E_USHER ? 300 : 100;
    ink_burst(cx, cy, 10, 260); sfx("poof");
    gore_enemy(e);
    if (e->type == E_BLOB) for (int k = -1; k <= 1; k += 2) {   /* blobs split in two */
        for (int i = 0; i < MAXE; i++) if (!en[i].alive && (en[i].type == E_MINIBLOB || i >= nen)) {
            if (i >= nen) nen = i + 1;
            Enemy *m = &en[i]; memset(m, 0, sizeof *m); m->type = E_MINIBLOB; m->alive = 1; m->hp = 1;
            m->x = e->x + k * 20; m->y = e->y - 4; m->vx = k * 180; m->vy = -400; m->timer = 0.6f; m->plat0 = 0; m->plat1 = LW * TILE; break;
        }
    }
}

static void update_enemies(void) {
    float speed_k = (darkness_on ? 1.35f : 1.0f) * (loop_no ? 1.25f : 1.0f) * (difficulty() == 2 ? 1.15f : difficulty() == 0 ? 0.85f : 1.0f);
    for (int i = 0; i < nen; i++) {
        Enemy *e = &en[i]; if (!e->alive) continue;
        if (e->x < camx - 500 || e->x > camx + SCREEN_W + 400) continue;   /* only near the screen */
        if (e->flash > 0) e->flash--;
        if (e->marked > 0) e->marked -= DT;
        if (lit[zone_of(e->x)] > 7.0f) e->stun = 1.0f;     /* the lights just slammed on: dazzled */
        if (e->stun > 0) { e->stun -= DT; continue; }
        float w, h; enemy_box(e, &w, &h);
        float dist; Player *t = nearest_player(e->x, e->y, &dist);
        e->t += DT * speed_k;
        if (e->type == E_SLICE || e->type == E_UMBRELLA || e->type == E_PUFF || e->type == E_CROW) {   /* flies: bob around home, then dive-bomb */
            float dive = e->type == E_CROW ? 640 : e->type == E_PUFF ? 420 : 480;
            if (e->state == 0) {
                e->face = t && t->x < e->x ? -1 : 1;
                float hx = t ? t->x - e->face * 220 : e->x;
                e->vx += ((hx - e->x) * 1.2f - e->vx) * 2 * DT; e->vx = clampf(e->vx, -200, 200);
                e->y = e->plat0 + sinf(e->t * 3) * 26;
                e->timer -= DT * speed_k;
                if (t && dist < 520 && e->timer <= 0) { e->state = 1; float dx = t->x - e->x, dy = (t->y - 70) - e->y, l = sqrtf(dx * dx + dy * dy) + 1;
                    e->vx = dx / l * dive * speed_k; e->vy = dy / l * dive * speed_k; sfx("spit"); }
            } else {
                e->vy -= 300 * DT;   /* pull out of the dive */
                if (e->y < e->plat0 && e->vy < 0) { e->state = 0; e->vy = 0; e->timer = 1.6f + frand(); }
            }
            e->x += e->vx * DT; if (e->state == 1) e->y += e->vy * DT;
            if (e->y > FLOOR_Y - 20) { e->y = FLOOR_Y - 20; e->vy = -300; }
            for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && overlap(p->x, p->y, 40, 110, e->x, e->y, 60, 46)) hurt_player(p, e->x); }
            continue;
        }
        if (e->type == E_EEL) {   /* hides in the ink; when you come close it rises and lunges */
            e->timer -= DT * speed_k;
            if (t) e->face = t->x < e->x ? -1 : 1;
            if (e->state == 0) { if (frame_no % 20 == 0) part_k(PK_BLOOD, e->x + (frand() - 0.5f) * 30, FLOOR_Y - 4, 0, -80, 0.4f, 4, 600, 20, 16, 20, 0);
                if (e->timer <= 0 && t && fabsf(t->x - e->x) < 300) { e->state = 1; e->timer = 0.45f; sfx("splash"); } }
            else if (e->state == 1) { if (e->timer <= 0) { e->state = 2; e->timer = 1.3f; sfx("spit"); } }
            else if (e->state == 2) {
                if (e->timer < 0.6f && e->timer > 0.25f) for (int k = 0; k < 2; k++) { Player *p = &pl[k];   /* the lunge reaches out */
                    if (p->active && p->alive && !p->hidden && overlap(p->x, p->y, 40, 110, e->x + e->face * 70, e->y - 90, 120, 110)) hurt_player(p, e->x); }
                if (e->timer <= 0) { e->state = 3; e->timer = 0.4f; } }
            else if (e->timer <= 0) { e->state = 0; e->timer = 1.6f + frand(); }
            if (e->state == 2) for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && overlap(p->x, p->y, 40, 110, e->x, e->y, 50, 150)) hurt_player(p, e->x); }
            continue;
        }
        if (e->type == E_GHOST) {   /* floats after you, fades in and out of the picture, lunges when solid */
            e->timer -= DT * speed_k;
            if (e->state == 0) {
                if (t) { e->face = t->x < e->x ? -1 : 1;
                    float hx = t->x - e->face * 200, hy = t->y - 110;
                    e->vx += ((hx - e->x) * 1.0f - e->vx) * 2 * DT; e->vy += ((hy - e->y) * 1.0f - e->vy) * 2 * DT;
                    e->vx = clampf(e->vx, -170, 170); e->vy = clampf(e->vy, -140, 140);
                    if (!ghost_faded(e) && dist < 460 && e->timer <= 0) { e->state = 1; e->timer = 0.7f;
                        float dx = t->x - e->x, dy = (t->y - 40) - e->y, l = sqrtf(dx * dx + dy * dy) + 1;
                        e->vx = dx / l * 460 * speed_k; e->vy = dy / l * 460 * speed_k; sfx("fiendwarn"); } }
                e->y += sinf(e->t * 2.5f) * 0.6f;
            } else if (e->timer <= 0) { e->state = 0; e->timer = 2.0f + frand(); e->vx *= 0.3f; e->vy *= 0.3f; }
            e->x += e->vx * DT; e->y += e->vy * DT;
            if (e->y > FLOOR_Y) { e->y = FLOOR_Y; e->vy = -100; } if (e->y < 140) e->y = 140;
            if (frame_no % 5 == 0 && !ghost_faded(e)) part(e->x - e->face * 30, e->y - 20, -e->face * 40, 20, 0.6f, 5, 0, 190, 190, 186, 0);
            if (!ghost_faded(e)) for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && overlap(p->x, p->y, 40, 110, e->x, e->y, 60, 90)) hurt_player(p, e->x); }
            continue;
        }
        switch (e->type) {
        case E_USHER:
            if (t && dist < 760) e->face = t->x < e->x ? -1 : 1;
            if (e->state == 0) {
                e->vx = (t && dist < 760 && fabsf(t->x - e->x) > 120) ? e->face * 75 * speed_k : 0;
                e->timer -= DT * speed_k;
                if (e->timer <= 0 && t && dist < 700) { e->state = 1; e->timer = 0.5f; e->vx = 0; }
            } else {
                e->timer -= DT * speed_k;
                if (e->timer <= 0) {
                    Proj *q = proj(P_LIGHT, e->x + e->face * 40, e->y - 98, e->face * 430, 0, 1, 1); if (q) q->r = 12;
                    sfx("spit"); e->state = 0; e->timer = 2.4f + frand();
                }
            }
            break;
        case E_DOG:
            if (e->state == 0) {                     /* patrol */
                if (e->vx == 0) e->vx = e->face * 90;
                e->vx = signf(e->vx) * 90 * speed_k; e->face = e->vx < 0 ? -1 : 1;
                if (e->x < e->plat0) { e->x = e->plat0; e->vx = 90; } if (e->x > e->plat1) { e->x = e->plat1; e->vx = -90; }
                e->timer -= DT;
                if (t && fabsf(t->x - e->x) < 620 && fabsf(t->y - e->y) < 170 && e->timer <= 0) {
                    e->state = 1; e->timer = 0.9f / speed_k; e->face = t->x < e->x ? -1 : 1; e->vx = 0; sfx("charge"); }
            } else if (e->state == 1) {              /* lens warming up */
                e->vx = 0; e->timer -= DT; if (e->timer <= 0) { e->state = 2; e->timer = 0.45f; sfx("beam"); }
            } else if (e->state == 2) {              /* projector beam */
                e->timer -= DT;
                float bx0 = e->x + e->face * 50, by = e->y - 76;
                {   /* where the beam ends: a wall, or 700 px out */
                    float ex = bx0; for (int k = 0; k < 70; k++) { ex += e->face * 10; if (solid_at((int)(ex / TILE), (int)(by / TILE))) break; }
                    if (frame_no % 2 == 0) part(ex, by, (frand() - 0.5f) * 300, -100 - frand() * 200, 0.3f, 4, 600, 255, 240, 180, 1);
                    if (frame_no % 10 == 0) { smoke(ex, by, 1, 60); splat(ex, by + (frand() - 0.5f) * 10, 7, 0, 30, 20, 12); }
                }
                for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (!p->active || !p->alive) continue;
                    float px0 = fminf(bx0, bx0 + e->face * 700), px1 = fmaxf(bx0, bx0 + e->face * 700);
                    if (p->x > px0 && p->x < px1 && by > p->y - (p->crouch ? 80 : 124) && by < p->y) hurt_player(p, e->x); }
                if (e->timer <= 0) { e->state = 0; e->timer = 2.0f; e->vx = e->face * 90; }
            }
            break;
        case E_GOAT:   /* like the usherettes, but the accordion fires a fan of confetti bombs */
            if (t && dist < 760) e->face = t->x < e->x ? -1 : 1;
            if (e->state == 0) {
                e->vx = (t && dist < 760 && fabsf(t->x - e->x) > 160) ? e->face * 85 * speed_k : 0;
                e->timer -= DT * speed_k;
                if (e->timer <= 0 && t && dist < 700) { e->state = 1; e->timer = 0.55f; e->vx = 0; }
            } else {
                e->timer -= DT * speed_k;
                if (e->timer <= 0) {
                    for (int k = 0; k < 3; k++) { Proj *q = proj(P_CONFETTI, e->x + e->face * 40, e->y - 90, e->face * (220 + k * 120), -420 - k * 60, 1, 1); if (q) { q->grav = 1100; q->r = 11; } }
                    sfx("boing"); e->state = 0; e->timer = 2.6f + frand();
                }
            }
            break;
        case E_MEATBALL:   /* rolls at you, then pounces */
            if (t && dist < 900) {
                if (e->state == 0) {
                    e->face = t->x < e->x ? -1 : 1;
                    float target = e->face * 230 * speed_k; e->vx += (target - e->vx) * 2.5f * DT;
                    if (e->onground && fabsf(t->x - e->x) < 170 && e->timer <= 0) { e->state = 1; e->vy = -520; e->vx = e->face * 260; e->timer = 1.4f; }
                } else if (e->onground && e->vy >= 0) e->state = 0;
            }
            e->timer -= DT;
            if (e->onground && frame_no % 6 == 0 && fabsf(e->vx) > 60) part_k(PK_BLOOD, e->x, e->y - 6, -e->vx * 0.3f, -80, 0.5f, 3, 1200, 160, 20, 14, 0);
            break;
        case E_BOMB:   /* hops at you; up close the fuse burns down and it goes off */
            if (frame_no % 3 == 0) part(e->x + e->face * 10, e->y - 84, (frand() - 0.5f) * 80, -100 - frand() * 80, 0.25f, 4, 300, 255, 230, 150, 1);
            if (e->state == 0) {
                e->timer -= DT * speed_k;
                if (e->onground) { e->vx *= 0.85f;
                    if (t && fabsf(t->x - e->x) < 120 && fabsf(t->y - e->y) < 130) { e->state = 1; e->timer = 0.8f; sfx("tick"); }
                    else if (e->timer <= 0 && t && dist < 800) { e->face = t->x < e->x ? -1 : 1; e->vx = e->face * 210; e->vy = -500; e->timer = 0.8f + frand() * 0.4f; } }
            } else {
                e->vx *= 0.8f; e->timer -= DT;
                if (frame_no % 8 == 0) sfx("tick");
                if (e->timer <= 0) { e->alive = 0; score += 50; gore_enemy(e); continue; }
            }
            break;
        case E_SHADOW:   /* an ink copy of you: runs, jumps and fires nails back */
            if (t) {
                e->face = t->x < e->x ? -1 : 1; float d = fabsf(t->x - e->x);
                float want = d > 340 ? e->face * 210 : d < 200 ? -e->face * 160 : 0; e->vx += (want * speed_k - e->vx) * 6 * DT;
                e->timer -= DT * speed_k;
                if (e->onground && rand() % 140 == 0) e->vy = -820;
                if (e->timer <= 0 && dist < 760) { Proj *q = proj(P_NAIL, e->x + e->face * 40, e->y - 82, e->face * 620, 0, 1, 1); if (q) q->r = 7; sfx("throw"); e->timer = 0.9f + frand() * 0.6f; }
            }
            break;
        case E_MANNEQUIN: {   /* only moves while you're not looking at it */
            int watched = 0;
            for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (!p->active || !p->alive || p->hidden) continue;
                if (fabsf(p->x - e->x) < SCREEN_W * 0.7f && ((p->face > 0) == (e->x > p->x))) watched = 1; }
            if (darkness_on) watched = 0;
            if (watched && e->state != 0) { e->state = 0; e->vx = 0; sfx("tick"); }
            if (!watched && t && dist < 900) {
                e->face = t->x < e->x ? -1 : 1;
                if (e->state == 0) e->state = 1;
                if (e->state == 1) { e->vx = e->face * 175 * speed_k; if (e->onground && fabsf(t->x - e->x) < 190) { e->state = 2; e->vy = -380; e->vx = e->face * 430; sfx("charge"); } }
                else if (e->state == 2 && e->onground && e->vy >= 0) e->state = 1;
            } else if (watched) e->vx = 0;
            break; }
        case E_MIRROR:   /* your reflection: mirrors your steps, jumps when you jump, shoots when you shoot */
            if (t && fabsf(t->x - e->mx) < 680) {
                float target = 2 * e->mx - t->x, side = e->plat0 != 0 ? e->plat0 : (e->x > e->mx ? 1 : -1);
                if (side > 0) target = fmaxf(target, e->mx + 36); else target = fminf(target, e->mx - 36);
                e->vx = clampf((target - e->x) * 10, -440, 440);
                e->face = -t->face;
                if (!t->onground && t->vy < -500 && e->onground) e->vy = t->vy;
                e->timer -= DT * speed_k;
                if (t->shoot_t > 0.06f && e->timer <= 0) { Proj *q = proj(P_NAIL, e->x + e->face * 40, e->y - 82, e->face * 600, 0, 1, 1); if (q) q->r = 7; sfx("throw"); e->timer = 0.34f; }
            } else e->vx = 0;
            break;
        case E_BULB:   /* walks in, charges up and sends a spark crackling along the floor: jump it */
            if (t && dist < 800) e->face = t->x < e->x ? -1 : 1;
            if (e->state == 0) {
                e->vx = (t && dist < 800 && fabsf(t->x - e->x) > 260) ? e->face * 90 * speed_k : 0;
                e->timer -= DT * speed_k;
                if (e->timer <= 0 && t && dist < 640 && fabsf(t->y - e->y) < 120) { e->state = 1; e->timer = 0.6f; e->vx = 0; sfx("charge"); }
            } else if (e->state == 1) {
                e->vx = 0; e->timer -= DT * speed_k;
                if (frame_no % 3 == 0) part(e->x + (frand() - 0.5f) * 40, e->y - 70, (frand() - 0.5f) * 200, -100 - frand() * 150, 0.3f, 3, 600, 255, 240, 140, 1);
                if (e->timer <= 0) { Proj *q = proj(P_SPARK, e->x + e->face * 30, e->y - 14, e->face * 380, 0, 1, 1); if (q) { q->r = 12; q->life = 3; }
                    sfx("beam"); e->state = 2; e->timer = 0.35f; }
            } else { e->vx = 0; e->timer -= DT; if (e->timer <= 0) { e->state = 0; e->timer = 2.0f + frand(); } }
            break;
        case E_SUITCASE:   /* big chomping hops toward you */
            e->timer -= DT * speed_k;
            if (e->onground) { e->vx *= 0.8f; if (e->timer <= 0 && t && dist < 820) {
                e->face = t->x < e->x ? -1 : 1; e->vx = e->face * 240; e->vy = -620; e->timer = 0.9f + frand() * 0.5f; sfx("boing"); } }
            break;
        case E_SOLDIER:   /* marches in, kneels and fires corks low along the floor: jump them */
            if (t && dist < 800) e->face = t->x < e->x ? -1 : 1;
            if (e->state == 0) {
                e->vx = (t && dist < 800 && fabsf(t->x - e->x) > 280) ? e->face * 80 * speed_k : 0;
                e->timer -= DT * speed_k;
                if (e->timer <= 0 && t && dist < 700 && fabsf(t->y - e->y) < 100) { e->state = 1; e->timer = 0.5f; e->vx = 0; }
            } else {
                e->timer -= DT * speed_k;
                if (e->timer <= 0) {
                    Proj *q = proj(P_CORK, e->x + e->face * 46, e->y - 34, e->face * 400, 0, 1, 1); if (q) q->r = 10;
                    smoke(e->x + e->face * 50, e->y - 36, 2, 220); sfx("throw"); e->state = 0; e->timer = 1.8f + frand();
                }
            }
            break;
        case E_IMP:   /* waddles near, then lobs a blob of ink */
            if (t && dist < 760) e->face = t->x < e->x ? -1 : 1;
            if (e->state == 0) {
                e->vx = (t && dist < 760 && fabsf(t->x - e->x) > 200) ? e->face * 95 * speed_k : 0;
                e->timer -= DT * speed_k;
                if (e->timer <= 0 && t && dist < 650) { e->state = 1; e->timer = 0.5f; e->vx = 0; }
            } else {
                e->timer -= DT * speed_k;
                if (e->timer <= 0 && t) {
                    float dx = t->x - e->x, tt = 0.8f;
                    Proj *q = proj(P_INKBLOB, e->x + e->face * 20, e->y - 80, clampf(dx / tt, -520, 520), -560, 1, 1); if (q) { q->grav = 1300; q->r = 13; }
                    sfx("spit"); e->state = 0; e->timer = 2.2f + frand();
                }
            }
            break;
        case E_WORKER:   /* clanks along; when it sees you it revs the jackhammer and charges */
            if (e->state == 0) {
                if (e->vx == 0) e->vx = e->face * 70;
                e->vx = signf(e->vx) * 70 * speed_k; e->face = e->vx < 0 ? -1 : 1;
                e->timer -= DT;
                if (t && fabsf(t->x - e->x) < 560 && fabsf(t->y - e->y) < 120 && e->timer <= 0) { e->state = 1; e->timer = 0.7f; e->face = t->x < e->x ? -1 : 1; e->vx = 0; sfx("charge"); }
            } else if (e->state == 1) {
                e->vx = 0; e->timer -= DT; if (frame_no % 4 == 0) smoke(e->x - e->face * 30, e->y - 110, 1, 200);
                if (e->timer <= 0) { e->state = 2; e->timer = 1.1f; sfx("clank"); }
            } else {
                e->vx = e->face * 430 * speed_k; e->timer -= DT;
                if (frame_no % 3 == 0) part(e->x + e->face * 50, e->y - 30, (frand() - 0.5f) * 200, -100 - frand() * 150, 0.3f, 3, 900, 255, 220, 140, 1);
                if (e->timer <= 0) { e->state = 0; e->timer = 1.4f; }
            }
            break;
        case E_GLOVE:   /* scuttles on its fingertips, crouches, then leaps in with a punch */
            if (t && dist < 900) {
                if (e->state == 0) { e->face = t->x < e->x ? -1 : 1; e->vx = e->face * 125 * speed_k; e->timer -= DT;
                    if (e->onground && fabsf(t->x - e->x) < 340 && e->timer <= 0) { e->state = 1; e->timer = 0.4f; e->vx = 0; } }
                else if (e->state == 1) { e->vx = 0; e->timer -= DT; if (e->timer <= 0) { e->state = 2; e->vy = -640; e->vx = e->face * 440 * speed_k; sfx("jump"); } }
                else if (e->state == 2) { if (e->onground && e->vy >= 0) { e->state = 3; e->timer = 0.45f; e->vx = 0; if (shake < 4) shake = 4; smoke(e->x, e->y, 3, 150); } }
                else { e->vx = 0; e->timer -= DT; if (e->timer <= 0) { e->state = 0; e->timer = 1.0f; } }
            } else e->vx = 0;
            break;
        case E_BLOB: case E_MINIBLOB:
            e->timer -= DT * speed_k;
            if (e->onground) { e->vx *= 0.85f; if (e->timer <= 0 && t && dist < 800) {
                e->face = t->x < e->x ? -1 : 1; e->vx = e->face * (e->type == E_BLOB ? 170 : 220); e->vy = -430; e->timer = 0.9f + frand() * 0.5f; } }
            break;
        default: e->vx = 0; break;
        }
        e->vy += 2200 * DT; if (e->vy > 1200) e->vy = 1200;
        int was_ground = e->onground;
        int r = move_box(&e->x, &e->y, &e->vx, &e->vy, w, h, 0);
        e->onground = (r & 1) != 0;
        if (!was_ground && e->onground && (e->type == E_BOMB || e->type == E_SUITCASE)) { e->land = 0.15f; if (e->type == E_SUITCASE) sfx("slam"); }
        if (!was_ground && e->onground && (e->type == E_BLOB || e->type == E_MINIBLOB)) { e->land = 0.18f; part_k(PK_BLOOD, e->x, e->y - 6, -80, -120, 0.6f, 4, 1200, 12, 8, 12, 0); part_k(PK_BLOOD, e->x, e->y - 6, 80, -120, 0.6f, 4, 1200, 12, 8, 12, 0); }
        if (e->land > 0) e->land -= DT;
        if ((r & 2) && (e->type == E_DOG || e->type == E_MEATBALL)) e->vx = -e->vx * 0.8f;
        if ((r & 2) && e->type == E_WORKER) { if (e->state == 2) { e->state = 0; e->timer = 1.4f; shake = 6; spark_burst(e->x + e->face * 40, e->y - 60, 12, 255, 220, 140); sfx("clank"); } e->vx = -e->face * 70; e->face = -e->face; }
        conveyor_carry(&e->x, &e->y, w, h);
        if (e->y > FLOOR_Y + 40) { e->alive = 0; ink_burst(e->x, FLOOR_Y, 10, 200); continue; }
        if (e->type != E_CRATE && e->type != E_HATCRATE)
            for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && overlap(p->x, p->y, 40, 110, e->x, e->y, w * 0.8f, h * 0.85f)) hurt_player(p, e->x); }
    }
    for (int i = 0; i < ndrip; i++) {   /* ink dripping from the rafters */
        drips[i].t -= DT; float x = drips[i].tx * TILE + 24;
        if (drips[i].t <= 0 && x > camx - 50 && x < camx + SCREEN_W + 50) { Proj *q = proj(P_DRIP, x, 10, 0, 60, 1, 1); if (q) { q->grav = 1300; q->r = 10; } drips[i].t = 1.9f; }
    }
}


/* brick ovens: every few seconds a fire column roars up out of the top. The security office's LIGHTS
   (which also kicks the kitchen breakers) shut the ovens off in that zone */
static float oven_phase(int tx) { return fmodf(scene_t + tx * 0.37f, 3.4f); }
static int oven_burning(int tx) { return lit[zone_of(tx * TILE)] <= 0 && oven_phase(tx) < 1.1f; }
static void update_ovens(void) {
    int t0 = (int)(camx / TILE) - 2, t1 = t0 + SCREEN_W / TILE + 4;
    for (int tx = t0; tx <= t1; tx++) { if (tx < 1 || tx >= LW) continue;
        for (int ty = 1; ty < LH; ty++) { if (map[ty][tx] != 'O' || map[ty - 1][tx] == 'O' || map[ty][tx - 1] == 'O') continue;
            if (!oven_burning(tx)) break;
            float fx = tx * TILE + TILE, top = ty * TILE;
            if (oven_phase(tx) < 0.05f) sfx("sizzle");
            if (frame_no % 2 == 0) { part(fx + (frand() - 0.5f) * 60, top - frand() * 150, (frand() - 0.5f) * 60, -260, 0.3f, 12, -200, 255, 140 + rand() % 80, 40, 1); }
            if (frame_no % 5 == 0) smoke(fx, top - 160, 1, 60);
            for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && overlap(p->x, p->y, 40, 120, fx, top, 80, 170)) hurt_player(p, fx); }
            break;
        }
    }
}
/* sprocket saws: spinning film sprockets with teeth. The ones up in the air swing up and down */
static float saw_y(int tx, int ty) { return ty * TILE + 24 + (ty < 12 ? sinf(scene_t * 2.0f + tx) * 60 : 0); }
static void update_saws(void) {
    int t0 = (int)(camx / TILE) - 2, t1 = t0 + SCREEN_W / TILE + 4;
    for (int tx = t0; tx <= t1; tx++) { if (tx < 1 || tx >= LW) continue;
        for (int ty = 0; ty < LH; ty++) { if (map[ty][tx] != 'S') continue;
            float cx = tx * TILE + 24, cy = saw_y(tx, ty);
            if (frame_no % 7 == 0) part(cx + (frand() - 0.5f) * 40, cy + 26, (frand() - 0.5f) * 300, -150 - frand() * 150, 0.3f, 3, 900, 255, 250, 230, 1);
            for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && overlap(p->x, p->y, 40, 110, cx, cy + 30, 60, 60)) hurt_player(p, cx); }
        } }
}
/* ceiling crushers: a riveted piston slams to the floor on a timer (LIGHTS in the zone shuts them down) */
static float crush_phase(int tx) { return fmodf(scene_t + tx * 0.53f, 3.2f); }
static float crusher_bottom(int tx) {   /* y of the piston head's underside */
    float ph = crush_phase(tx), up = TILE * 1.0f + 20, down = FLOOR_Y;
    if (lit[zone_of(tx * TILE)] > 0) return up;
    if (ph < 2.0f) return up + (ph > 1.6f ? sinf(frame_no * 1.3f) * 3 : 0);
    if (ph < 2.14f) return up + (down - up) * ((ph - 2.0f) / 0.14f);
    if (ph < 2.7f) return down;
    return down - (down - up) * ((ph - 2.7f) / 0.5f);
}
/* steam vents: a blast of steam that throws you high into the air (that's how you reach the high catwalks) */
static int vent_on(int tx) { return fmodf(scene_t + tx * 0.7f, 2.6f) < 0.9f; }
static void update_crushers_vents(void) {
    int t0 = (int)(camx / TILE) - 2, t1 = t0 + SCREEN_W / TILE + 4;
    for (int tx = t0; tx <= t1; tx++) { if (tx < 1 || tx >= LW) continue;
        if (map[0][tx] == 'X') {
            float b = crusher_bottom(tx), ph = crush_phase(tx), cx = tx * TILE + 24;
            if (lit[zone_of(tx * TILE)] <= 0 && ph >= 2.14f && ph < 2.14f + DT * 1.5f) { sfx("slam"); if (cx > camx && cx < camx + SCREEN_W) { shake = fmaxf(shake, 7); smoke(cx, FLOOR_Y - 6, 4, 120); for (int k = 0; k < 6; k++) part_k(PK_DEBRIS, cx, FLOOR_Y - 6, (frand() - 0.5f) * 500, -150 - frand() * 300, 1.5f, 5, 1600, 60, 50, 44, 0); } }
            for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && fabsf(p->x - cx) < 56 && b > p->y - 120 && ph > 2.0f && ph < 2.7f) hurt_player(p, cx); }
            for (int j = 0; j < nen; j++) { Enemy *e = &en[j]; if (e->alive && e->type < E_CRATE && e->type != E_EEL && fabsf(e->x - cx) < 56 && b > e->y - 40 && ph > 2.0f && ph < 2.7f) enemy_damage(e, 99); }
        }
        if (map[12][tx] == 'V' && vent_on(tx)) {
            float cx = tx * TILE + 24;
            if (frame_no % 2 == 0) part_k(PK_SMOKE, cx + (frand() - 0.5f) * 30, FLOOR_Y - 20, (frand() - 0.5f) * 60, -500 - frand() * 200, 0.7f, 14, 0, 230, 230, 225, 0);
            if (fmodf(scene_t + tx * 0.7f, 2.6f) < DT * 1.5f && cx > camx && cx < camx + SCREEN_W) sfx("sizzle");
            for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && fabsf(p->x - cx) < 46 && p->y > FLOOR_Y - 380 && p->vy > -900) { p->vy = -1250; p->onground = 0; } }
        }
    }
}
/* toy blocks crumble a moment after you land on them, then stack themselves back up; mousetraps snap */
static void update_blocks_traps(void) {
    if (level_no != L_LOST && level_no != L_ENCORE) return;
    for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (!p->active || !p->alive || !p->onground) continue;
        int ty = (int)((p->y + 2) / TILE);
        for (int dx = -1; dx <= 1; dx++) { int tx = (int)((p->x + dx * 16) / TILE);
            if (tx >= 0 && tx < LW && ty >= 0 && ty < LH && map[ty][tx] == 'U' && crumble_t[ty][tx] == 0) { crumble_t[ty][tx] = 0.01f; sfx("tick"); } } }
    int t0 = (int)(camx / TILE) - 4, t1 = t0 + SCREEN_W / TILE + 8;
    for (int ty = 0; ty < LH; ty++) for (int tx = t0; tx <= t1; tx++) { if (tx < 0 || tx >= LW) continue;
        if (map[ty][tx] == 'U' && crumble_t[ty][tx] > 0) { crumble_t[ty][tx] += DT;
            if (crumble_t[ty][tx] > 0.55f) { map[ty][tx] = 'u'; crumble_t[ty][tx] = 0; regrow_t[ty][tx] = 4.0f; sfx("poof");
                for (int i = 0; i < 4; i++) part_k(PK_DEBRIS, tx * TILE + 24, ty * TILE + 10, (frand() - 0.5f) * 300, -100 - frand() * 200, 2.0f, 10 + frand() * 6, 1600, 180 + rand() % 60, 60 + rand() % 120, 50 + rand() % 80, 0); } }
        else if (map[ty][tx] == 'u') { regrow_t[ty][tx] -= DT;
            int blocked = 0; for (int k = 0; k < 2; k++) if (pl[k].active && fabsf(pl[k].x - (tx * TILE + 24)) < 40 && fabsf(pl[k].y - ty * TILE) < 60) blocked = 1;
            if (regrow_t[ty][tx] <= 0 && !blocked) { map[ty][tx] = 'U'; sfx("pickup"); } }
    }
    for (int tx = t0; tx <= t1; tx++) { if (tx < 0 || tx >= LW || map[12][tx] != 'M') continue;
        float cx = tx * TILE + 24;
        if (trap_t[tx] == 0) { for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && fabsf(p->x - cx) < 46 && p->y > FLOOR_Y - 40) { trap_t[tx] = 0.01f; sfx("tick"); } } }
        else { float before = trap_t[tx]; trap_t[tx] += DT;
            if (before < 0.28f && trap_t[tx] >= 0.28f) { sfx("slam"); shake = fmaxf(shake, 5); spark_burst(cx, FLOOR_Y - 20, 8, 255, 230, 180);
                for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && fabsf(p->x - cx) < 60 && p->y > FLOOR_Y - 70) hurt_player(p, cx); }
                for (int j = 0; j < nen; j++) { Enemy *e = &en[j]; if (e->alive && e->type < E_CRATE && fabsf(e->x - cx) < 60 && e->y > FLOOR_Y - 70) enemy_damage(e, 3); } }
            if (trap_t[tx] > 2.2f) trap_t[tx] = 0; }
    }
}
/* the orchestra pit floods with ink in waves (Show 6) */
static float tide_surface(void) {   /* y of the ink's surface in the pit zone */
    float ph = fmodf(scene_t, 9.0f), low = FLOOR_Y + 30, high = FLOOR_Y - 120;
    if (ph < 4.0f) return low; if (ph < 5.0f) return low + (high - low) * (ph - 4.0f);
    if (ph < 8.0f) return high + sinf(scene_t * 3) * 4; return high + (low - high) * (ph - 8.0f);
}
static void update_tide(void) {
    if (level_no != L_ENCORE) return;
    float x0 = ZONE_X0[2] * TILE, x1 = ZONE_X0[3] * TILE, surf = tide_surface(), ph = fmodf(scene_t, 9.0f);
    int in = 0; for (int k = 0; k < 2; k++) if (pl[k].active && pl[k].x > x0 && pl[k].x < x1) in = 1;
    if (in && ph >= 3.0f && ph < 3.0f + DT * 1.5f) { message("THE INK IS RISING! GET UP HIGH!", 1.6f); sfx("fiendwarn"); }
    if (in && ph >= 4.0f && ph < 4.0f + DT * 1.5f) sfx("splash");
    for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && p->x > x0 && p->x < x1 && p->y > surf + 12) hurt_player(p, p->x + 1); }
    if (in && surf < FLOOR_Y && frame_no % 3 == 0) part_k(PK_BLOOD, x0 + frand() * (x1 - x0), surf, (frand() - 0.5f) * 80, -100 - frand() * 120, 0.6f, 4, 900, 12, 8, 12, 0);
}
/* ------------------------------------------------------------ the Ink Fiend (backstage) */
static int any_player_in_zone(int z) { for (int i = 0; i < 2; i++) if (pl[i].active && pl[i].alive && zone_of(pl[i].x) == z) return 1; return 0; }
static void update_fiend(void) {
    if (level_no != 0) return;   /* the Ink Fiend lives backstage in Show 1 */
    int in_zone = any_player_in_zone(3);
    if (fiend.state == 0) {
        if (!in_zone) return;
        fiend.cycle -= DT;
        if (!fiend.seen) { fiend.seen = 1; fiend.cycle = 3.5f; message("SOMETHING IS MOVING BACKSTAGE...", 2.5f); }
        if (fiend.cycle <= 0) { fiend.state = 1; fiend.t = 1.8f; warn_t = 1.8f; sfx("fiendwarn"); }
        return;
    }
    if (fiend.state == 1) {
        fiend.t -= DT;
        if (fiend.t <= 0) { fiend.state = 2; fiend.x = camx - 120; fiend.y = FLOOR_Y; fiend.t = 0; sfx("roar1"); }
        return;
    }
    if (lit[3] > 0 && fiend.state == 2) { fiend.state = 3; sfx("roar2"); message("THE LIGHTS DROVE IT BACK!", 1.5f); }
    if (fiend.state == 2) {
        fiend.t += DT;
        float d; Player *t = nearest_player(fiend.x, fiend.y, &d);
        float dir = 1, spd = 395;
        if (t) dir = t->x < fiend.x ? -1 : 1; else spd = 300;
        if (!t && fiend.t > 1.0f) dir = 1;
        fiend.x += dir * spd * DT;
        if (frame_no % 3 == 0) part(fiend.x + (frand() - 0.5f) * 140, fiend.y - 60 - frand() * 160, 0, 40, 0.8f, 8 + frand() * 8, 300, 10, 8, 8, 0);
        for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && fabsf(p->x - fiend.x) < 90) hurt_player(p, fiend.x); }
        if (fiend.x > camx + SCREEN_W + 200 || fiend.t > 9.0f || !in_zone) { fiend.state = 3; }
    }
    if (fiend.state == 3) {
        fiend.x -= 600 * DT;
        if (fiend.x < camx - 300 || fiend.x > camx + SCREEN_W + 300) { fiend.state = 0; fiend.cycle = 7.0f + frand() * 3; }
    }
}

/* ------------------------------------------------------------ boss: Uncle Inky */
enum { BS_INTRO, BS_IDLE, BS_SLAM_AIM, BS_SLAM_HIT, BS_NOTES, BS_SWEEP_WARN, BS_SWEEP, BS_TICKETS, BS_DEAD, BS_WARP };
static void boss_start(void) {
    boss_hit = 0;
    sfx("boss_sting");
    boss.active = 1; boss.state = BS_INTRO; boss.timer = 2.6f; boss.maxhp = boss.hp = LEVELS[level_no].boss_hp; boss.face = -1;
    if (loop_no) boss.maxhp = boss.hp = boss.hp * 13 / 10;   /* second run: tougher bosses */
    if (difficulty() == 0) boss.maxhp = boss.hp = boss.hp * 7 / 10;
    if (difficulty() == 2) boss.maxhp = boss.hp = boss.hp * 13 / 10;
    if (getenv("MM_BOSS_HP")) boss.hp = atoi(getenv("MM_BOSS_HP"));   /* tests */
    boss.x = ARENA_X + SCREEN_W - 230; boss.y = FLOOR_Y;
    arena_locked = 1; music_play(LEVELS[level_no].boss_music, 1); sfx(level_no == 1 ? "angry" : "roar1"); shake = 10;
}
static void boss_hit_player_box(float x, float y, float w, float h) {
    for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && overlap(p->x, p->y, 40, p->crouch ? 80 : 120, x, y, w, h)) hurt_player(p, x); }
}
static void update_boss_rat(void);
static void update_boss_splicer(void);
static void update_boss_press(void);
static void update_boss_collector(void);
static void update_boss_unbound(void);
static void update_boss_vanity(void);
static void update_boss_headliner(void);
static void update_boss(void) {
    boss_entrance_update();
    if (!boss.active) {
        if (!arena_locked) for (int k = 0; k < 2; k++) if (pl[k].active && pl[k].alive && pl[k].x > ARENA_X + 150) boss_start();
        return;
    }
    if (level_no == L_ENCORE) { update_boss_unbound(); return; }
    if (level_no == L_ROOF) { update_boss_headliner(); return; }
    if (level_no == L_MIRROR) { update_boss_vanity(); return; }
    if (level_no == 4) { update_boss_collector(); return; }
    if (level_no == 3) { update_boss_press(); return; }
    if (level_no == 2) { update_boss_splicer(); return; }
    if (level_no == 1) { update_boss_rat(); return; }
    float angry = boss.hp < boss.maxhp / 2 ? 0.7f : 1.0f;
    boss.t += DT; boss.bob = sinf(boss.t * 2.2f) * 10;
    if (boss.flash > 0) boss.flash--;
    boss.timer -= DT;
    Player *t = nearest_player(boss.x, boss.y, NULL);
    /* he turns to face you between attacks (never mid-swing, so every attack can be read) */
    if (t && (boss.state == BS_IDLE || boss.state == BS_NOTES)) boss.face = t->x < boss.x ? -1 : 1;
    switch (boss.state) {
    case BS_INTRO: boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f; } break;
    case BS_IDLE:
        boss.pose = angry < 1 ? 2 : 0;
        if (boss.timer <= 0 && t && fabsf(t->x - boss.x) < 300 && rand() % 3 != 0) {
            /* crowding him: he melts into an ink puddle and pours back up across the stage */
            boss.state = BS_WARP; boss.timer = 1.0f; boss.slams = 0; sfx("splat");
            float lo = ARENA_X + 230, hi = ARENA_X + SCREEN_W - 230;
            boss.targetx = fabsf(t->x - lo) > fabsf(t->x - hi) ? lo : hi;
            ink_burst(boss.x, FLOOR_Y - 20, 14, 260);
        }
        else if (boss.timer <= 0) {
            int pick_ = rand() % (angry < 1 ? 4 : 3);
            if (pick_ == 0) { boss.state = BS_SLAM_AIM; boss.slams = angry < 1 ? 4 : 3; boss.timer = 0.85f * angry; boss.targetx = t ? t->x : boss.x + boss.face * 400; }
            else if (pick_ == 1) { boss.state = BS_NOTES; boss.timer = 0.4f; boss.slams = 3; }
            else if (pick_ == 2) { boss.state = BS_SWEEP_WARN; boss.timer = 0.9f * angry; sfx("charge"); }
            else { boss.state = BS_TICKETS; boss.timer = 2.2f; boss.slams = 0; }
            if (angry < 1 && !boss.summoned) { boss.summoned = 1; spawn(E_BLOB, (int)((ARENA_X + 300) / TILE), 13); spawn(E_BLOB, (int)((ARENA_X + 700) / TILE), 13); sfx("summon"); }
        }
        break;
    case BS_SLAM_AIM:
        boss.pose = 1;
        if (boss.timer > 0.3f && t) boss.targetx += (t->x - boss.targetx) * 0.08f;   /* tracks you, then commits */
        if (boss.timer <= 0) {
            boss.state = BS_SLAM_HIT; boss.timer = 0.32f; shake = 14; sfx("slam"); hitstop = 2;
            ink_burst(boss.targetx, FLOOR_Y, 16, 300); ring(boss.targetx, FLOOR_Y - 10, 180, 255, 230, 190);
            for (int k = 0; k < 10; k++) part_k(PK_DEBRIS, boss.targetx + (frand() - 0.5f) * 60, FLOOR_Y - 8, (frand() - 0.5f) * 700, -300 - frand() * 500, 2.0f, 8 + frand() * 10, 1700, 110, 70, 40, 0);
            for (int k = 0; k < 12; k++) part_k(PK_BLOOD, boss.targetx, FLOOR_Y - 10, (frand() - 0.5f) * 600, -300 - frand() * 600, 1.4f, 5 + frand() * 6, 1500, 12, 8, 12, 0);
            smoke(boss.targetx, FLOOR_Y - 10, 5, 80);
        }
        break;
    case BS_SLAM_HIT:
        boss.pose = 1;
        boss_hit_player_box(boss.targetx, FLOOR_Y, 86, FLOOR_Y);
        if (boss.timer <= 0) {
            if (--boss.slams > 0) { boss.state = BS_SLAM_AIM; boss.timer = 0.6f * angry; boss.targetx = t ? t->x : boss.targetx; }
            else { boss.state = BS_IDLE; boss.timer = 1.3f * angry; boss.summoned = boss.hp < boss.maxhp / 2 ? boss.summoned : 0; }
        }
        break;
    case BS_NOTES:
        boss.pose = 0;
        if (boss.timer <= 0) {
            float hx = boss.x + boss.face * 120, tx = t ? t->x : boss.x + boss.face * 500, dx = tx - hx;
            Proj *q = proj(P_NOTE, hx, boss.y - 300, dx * (0.75f + 0.25f * boss.slams) / 1.1f, -700, 1, 1);
            if (q) { q->grav = 1300; q->r = 18; q->bounces = 2; q->life = 4; }
            sfx("spit"); boss.timer = 0.45f * angry;
            if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.4f * angry; }
        }
        break;
    case BS_SWEEP_WARN:
        boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_SWEEP; boss.sweepx = boss.x + boss.face * 140; sfx("beam"); } break;
    case BS_SWEEP:
        boss.sweepx += boss.face * 760 / angry * DT;
        if (frame_no % 2 == 0) part(boss.sweepx, FLOOR_Y - 6, (frand() - 0.5f) * 200, -150 - frand() * 250, 0.4f, 4, 900, 255, 230, 150, 1);
        if (frame_no % 5 == 0) splat(boss.sweepx, FLOOR_Y + 2, 12, 1, 40, 26, 14);
        boss_hit_player_box(boss.sweepx, FLOOR_Y, 70, 52);
        if (boss.sweepx < ARENA_X - 60 || boss.sweepx > ARENA_X + SCREEN_W + 60) { boss.state = BS_IDLE; boss.timer = 1.2f * angry; }
        break;
    case BS_WARP:   /* 1.0 -> 0.55 sinking, 0.55 -> 0.45 puddle slides, 0.45 -> 0 rising */
        boss.pose = 1;
        if (boss.timer < 0.55f && !boss.slams) { boss.slams = 1; boss.x = boss.targetx; ink_burst(boss.x, FLOOR_Y - 20, 14, 260); sfx("splat"); }
        if (frame_no % 3 == 0) splat(boss.x + (frand() - 0.5f) * 160, FLOOR_Y + 2, 14, 1, 20, 14, 24);
        if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 0.5f * angry; boss.slams = 0; }
        break;
    case BS_TICKETS:
        boss.pose = 2;
        if (frame_no % 9 == 0) { Proj *q = proj(P_TICKET, ARENA_X + 40 + frand() * (SCREEN_W - 80), -20, (frand() - 0.5f) * 60, 160, 1, 1); if (q) { q->r = 14; q->grav = 300; } }
        if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.2f; }
        break;
    case BS_DEAD:
        if (boss.pose >= 0) boss.pose = 2;
        if (frame_no % 4 == 0) { ink_burst(boss.x + (frand() - 0.5f) * 260, boss.y - frand() * 380, 8, 300); sfx("boom");
            blood_spurt(boss.x + (frand() - 0.5f) * 200, boss.y - frand() * 300, frand() < 0.5f ? -1 : 1, 10, 700);
            gore_part(rand() % 8, boss.x + (frand() - 0.5f) * 200, boss.y - 100 - frand() * 250, 700, rand() & 1);
            if (frame_no % 12 == 0) ring(boss.x + (frand() - 0.5f) * 200, boss.y - frand() * 300, 120, 255, 200, 120); }
        if (!boss.summoned && boss.timer < 1.0f) {   /* final blow: he comes apart, mask and all */
            boss.summoned = 1; shatter(&A.inky[2], boss.x, boss.y + 10, 0, 5, 5, 800, 1); boss.pose = -1;
            for (int k = 0; k < 6; k++) gore_part(2, boss.x - 30, boss.y - 330, 800, 0);
            for (int k = 0; k < 4; k++) gore_part(4, boss.x, boss.y - 200, 700, 1);
            gore_part(3, boss.x - 20, boss.y - 320, 900, 1); gore_part(3, boss.x + 20, boss.y - 320, 900, 1);
            blood_spurt(boss.x, boss.y - 250, -1, 60, 900); blood_spurt(boss.x, boss.y - 250, 1, 40, 800);
            ring(boss.x, boss.y - 200, 300, 255, 220, 160); shake = 24; hitstop = 10; rumble_all(1.0f, 650);
        }
        boss.y += 40 * DT;
        if (boss.timer <= 0 && scene == SC_PLAY) show_cleared();
        break;
    }
    /* body contact */
    if (boss.state != BS_DEAD && boss.state != BS_INTRO && boss.state != BS_WARP) boss_hit_player_box(boss.x, boss.y, 200, 300);
}


static float sx(float x);
static void draw_sprite(Tex *t, float x, float feet, float scale, int face_right, Uint8 flash, Uint8 alpha, double angle, float sy);
/* ---- Show 2 boss: BIG CHEESE, the rat chef ---- */
enum { RS_CHOP_AIM = 20, RS_CHOP, RS_PIZZA, RS_WAVE_WARN, RS_WAVE, RS_LEAP, RS_FALL };
enum { SS_SNIP_AIM = 40, SS_SNIP, SS_FILM, SS_REELS, SS_FLICK_OUT, SS_FLICK_IN };
enum { PS_STAMP_AIM = 60, PS_STAMP, PS_SHEETS, PS_ROLL_WARN, PS_ROLL };
enum { CS_STAB_AIM = 80, CS_STAB, CS_THROW, CS_SUCK };
enum { US_SLAM_AIM = 100, US_SLAM, US_RAIN, US_BEAM_WARN, US_BEAM };
static void update_boss_rat(void) {
    float angry = boss.hp < boss.maxhp / 2 ? 0.7f : 1.0f;
    boss.t += DT; if (boss.flash > 0) boss.flash--;
    boss.timer -= DT;
    Player *t = nearest_player(boss.x, boss.y, NULL);
    float lo = ARENA_X + 220, hi = ARENA_X + SCREEN_W - 180;
    if (t && boss.state != RS_LEAP && boss.state != RS_FALL && boss.state != BS_DEAD) boss.face = t->x < boss.x ? -1 : 1;
    switch (boss.state) {
    case BS_INTRO: boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f; } break;
    case BS_IDLE:
        boss.pose = angry < 1 ? 4 : 0; boss.bob = fabsf(sinf(boss.t * 6)) * -10;
        if (t) { float want = t->x - boss.face * 380; boss.x += clampf(want - boss.x, -1, 1) * 110 / angry * DT; }
        boss.x = clampf(boss.x, lo, hi);
        if (boss.timer <= 0) {
            int pick_ = rand() % (angry < 1 ? 4 : 3);
            if (pick_ == 0) { boss.state = RS_CHOP_AIM; boss.slams = angry < 1 ? 3 : 2; boss.timer = 0.8f * angry; boss.targetx = t ? t->x : boss.x; }
            else if (pick_ == 1) { boss.state = RS_PIZZA; boss.slams = 3; boss.timer = 0.3f; }
            else if (pick_ == 2) { boss.state = RS_WAVE_WARN; boss.timer = 0.8f * angry; sfx("charge"); }
            else { boss.state = RS_LEAP; boss.vy = -2100; sfx("jump"); }
            if (angry < 1 && !boss.summoned) { boss.summoned = 1; spawn(E_MEATBALL, (int)((ARENA_X + 200) / TILE), 13); spawn(E_MEATBALL, (int)((ARENA_X + 900) / TILE), 13); sfx("summon"); }
        }
        break;
    case RS_CHOP_AIM:   /* a cleaver shadow tracks you, then commits */
        boss.pose = 1;
        if (boss.timer > 0.28f && t) boss.targetx += (t->x - boss.targetx) * 0.09f;
        if (boss.timer <= 0) { boss.state = RS_CHOP; boss.timer = 0.3f; shake = 14; hitstop = 2; sfx("slam");
            ring(boss.targetx, FLOOR_Y - 10, 150, 255, 230, 190); spark_burst(boss.targetx, FLOOR_Y - 10, 20, 255, 240, 200);
            for (int k = 0; k < 8; k++) part_k(PK_DEBRIS, boss.targetx, FLOOR_Y - 8, (frand() - 0.5f) * 700, -300 - frand() * 500, 2.0f, 8 + frand() * 10, 1700, 110, 70, 40, 0); }
        break;
    case RS_CHOP:
        boss.pose = 2; boss_hit_player_box(boss.targetx, FLOOR_Y, 110, FLOOR_Y);
        if (boss.timer <= 0) { if (--boss.slams > 0) { boss.state = RS_CHOP_AIM; boss.timer = 0.55f * angry; boss.targetx = t ? t->x : boss.targetx; }
            else { boss.state = BS_IDLE; boss.timer = 1.3f * angry; } }
        break;
    case RS_PIZZA:      /* flings pizzas that bounce along the floor */
        boss.pose = boss.timer > 0.5f * angry - 0.22f && boss.slams < 3 ? 3 : 0;
        if (boss.timer <= 0) {
            Proj *q = proj(P_PIZZA, boss.x + boss.face * 120, boss.y - 260, boss.face * (330 + 70 * boss.slams), -520, 1, 1);
            if (q) { q->grav = 1400; q->r = 26; q->bounces = 4; q->life = 4; }
            sfx("boing"); boss.timer = 0.5f * angry;
            if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.2f * angry; }
        }
        break;
    case RS_WAVE_WARN: boss.pose = 1; if (boss.timer <= 0) { boss.state = RS_WAVE; boss.sweepx = boss.x + boss.face * 120; sfx("splash"); shake = 8; } break;
    case RS_WAVE:       /* a wave of hot cheese rolls across the floor: jump it */
        boss.sweepx += boss.face * 700 / angry * DT;
        boss_hit_player_box(boss.sweepx, FLOOR_Y, 80, 64);
        if (frame_no % 2 == 0) part_k(PK_BLOOD, boss.sweepx, FLOOR_Y - 30, (frand() - 0.5f) * 200, -200 - frand() * 200, 0.8f, 6, 1200, 250, 190, 60, 0);
        if (boss.sweepx < ARENA_X - 60 || boss.sweepx > ARENA_X + SCREEN_W + 60) { boss.state = BS_IDLE; boss.timer = 1.1f * angry; }
        break;
    case RS_LEAP:       /* jumps off the top of the screen... */
        boss.pose = 4; boss.vy += 1800 * DT; boss.y += boss.vy * DT;
        if (boss.y < -200 || (boss.vy > 0 && boss.y > -60)) { boss.state = RS_FALL; boss.timer = 1.0f; boss.targetx = t ? t->x : boss.x; boss.vy = 0; }
        break;
    case RS_FALL:       /* ...and crashes down where you were standing */
        boss.pose = 4;
        if (boss.timer > 0.25f && t) boss.targetx += (t->x - boss.targetx) * 0.06f;
        if (boss.timer <= 0) {
            boss.x = clampf(boss.targetx, lo, hi); boss.vy += 3200 * DT; boss.y += boss.vy * DT;
            if (boss.y >= FLOOR_Y) { boss.y = FLOOR_Y; boss.state = BS_IDLE; boss.timer = 1.4f; shake = 20; hitstop = 4; sfx("slam"); sfx("boom");
                ring(boss.x, FLOOR_Y - 10, 260, 255, 220, 160); smoke(boss.x, FLOOR_Y - 10, 8, 80);
                boss_hit_player_box(boss.x, FLOOR_Y, 260, 300);
                for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && p->onground && fabsf(p->x - boss.x) < 520) hurt_player(p, boss.x); }  /* shockwave: be in the air */
            }
        }
        break;
    case BS_DEAD:
        if (boss.pose >= 0) boss.pose = 4;
        if (frame_no % 4 == 0) { sfx("boom"); blood_spurt(boss.x + (frand() - 0.5f) * 200, boss.y - frand() * 300, frand() < 0.5f ? -1 : 1, 10, 700);
            gore_part2(rand() % 12, boss.x + (frand() - 0.5f) * 200, boss.y - 100 - frand() * 250, 700, rand() & 1);
            for (int k = 0; k < 6; k++) part_k(PK_BLOOD, boss.x, boss.y - 200, (frand() - 0.5f) * 800, -300 - frand() * 500, 1.4f, 6, 1500, 250, 190, 60, 0); }
        if (!boss.summoned && boss.timer < 1.0f) {
            boss.summoned = 1; shatter(&A.rat[4], boss.x, boss.y + 10, boss.face > 0, 5, 5, 800, 1); boss.pose = -1;
            for (int k = 0; k < 6; k++) gore_part2(11, boss.x, boss.y - 200, 800, 1);
            gore_part2(3, boss.x - 20, boss.y - 330, 900, 1); gore_part2(3, boss.x + 20, boss.y - 330, 900, 1);
            blood_spurt(boss.x, boss.y - 250, -1, 60, 900); blood_spurt(boss.x, boss.y - 250, 1, 40, 800);
            ring(boss.x, boss.y - 200, 300, 255, 220, 160); shake = 24; hitstop = 10; rumble_all(1.0f, 650);
        }
        if (boss.timer <= 0 && scene == SC_PLAY) show_cleared();
        break;
    }
    if (boss.state != BS_DEAD && boss.state != BS_INTRO && boss.state != RS_LEAP && boss.state != RS_FALL) boss_hit_player_box(boss.x, boss.y, 190, 300);
}
static void draw_boss_rat(float ox, float oy) {
    float x = sx(boss.x) + ox, y = boss.y + oy + boss.bob;
    if (boss.state == RS_CHOP_AIM) { float k = 1 - boss.timer / 0.8f, tx = sx(boss.targetx) + ox;
        draw_ellipse(&A.disc, tx, FLOOR_Y + oy, 40 + 30 * k, 9 + 6 * k, 10, 6, 10, (Uint8)(120 + 120 * k));
        if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", tx, FLOOR_Y + oy - 60, 0.8f, 1, 255, 60, 40, 255); }
    if (boss.state == RS_CHOP) { float tx = sx(boss.targetx) + ox;   /* his cleaver, giant, slams down blade-first */
        float k = clampf((0.3f - boss.timer) / 0.06f, 0, 1), h = A.cleaver.h ? (float)A.cleaver.h : 300;
        float cy = -h + (FLOOR_Y + oy + 16) * k;   /* bottom edge reaches the floor */
        if (A.cleaver.tex) draw_tex_srcx(&A.cleaver, NULL, tx - A.cleaver.w * 0.5f, cy, (float)A.cleaver.w, h, boss.face < 0, 0, 255, 255, 255, 255);
        else { fill_rect((int)tx - 50, 0, 100, FLOOR_Y + (int)oy - 16, 190, 196, 205, 255); } }
    if (boss.state == RS_FALL && boss.timer > 0) { float k = 1 - boss.timer, tx = sx(boss.targetx) + ox;
        draw_ellipse(&A.disc, tx, FLOOR_Y + oy, 60 + 120 * k, 12 + 12 * k, 10, 6, 10, (Uint8)(100 + 140 * k)); }
    if (boss.state == RS_WAVE_WARN && (frame_no / 5) % 2) draw_glow(&A.disc, x + boss.face * 120, FLOOR_Y + oy - 30, 70, 255, 200, 80, 255);
    if (boss.state == RS_WAVE) { float bx = sx(boss.sweepx) + ox;
        for (int k = 0; k < 6; k++) draw_ellipse(&A.disc, bx - boss.face * k * 22, FLOOR_Y + oy - 22 - (5 - k) * 6, 40 - k * 4, 30 - k * 3, 245, 190 - k * 10, 60, 255);
        draw_ellipse(&A.disc, bx - 8, FLOOR_Y + oy - 50, 8, 5, 255, 240, 200, 220); }
    if (boss.pose < 0) return;
    Tex *t = &A.rat[boss.pose];
    double ang = boss.state == BS_DEAD ? sinf(frame_no * 0.5f) * 6 : sinf(boss.t * 1.6f) * 2;
    draw_sprite(t, x, y + 10, 1, boss.face > 0, boss.flash ? 170 : 0, 255, ang, 1.0f + 0.02f * sinf(boss.t * 5));
}
/* ------------------------------------------------------------ boss: The Splicer (Show 3) */
static void update_boss_splicer(void) {
    float angry = boss.hp < boss.maxhp / 2 ? 0.7f : 1.0f;
    boss.t += DT; if (boss.flash > 0) boss.flash--;
    boss.timer -= DT; boss.bob = sinf(boss.t * 3) * 6;
    Player *t = nearest_player(boss.x, boss.y, NULL);
    float lo = ARENA_X + 250, hi = ARENA_X + SCREEN_W - 250;
    if (t && boss.state != BS_DEAD && boss.state != SS_FLICK_OUT) boss.face = t->x < boss.x ? -1 : 1;
    switch (boss.state) {
    case BS_INTRO: boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f; } break;
    case BS_IDLE:
        boss.pose = angry < 1 ? 2 : 0;
        if (t) { float want = t->x - boss.face * 360; boss.x += clampf(want - boss.x, -1, 1) * 150 / angry * DT; }
        boss.x = clampf(boss.x, lo, hi);
        if (boss.timer <= 0) {
            int k = rand() % 4;
            if (k == 0) { boss.state = SS_SNIP_AIM; boss.slams = angry < 1 ? 3 : 2; boss.timer = 0.75f * angry; boss.targetx = t ? t->x : boss.x; sfx("charge"); }
            else if (k == 1) { boss.state = SS_FILM; boss.slams = angry < 1 ? 5 : 3; boss.timer = 0.4f; }
            else if (k == 2) { boss.state = SS_REELS; boss.slams = angry < 1 ? 4 : 3; boss.timer = 0.3f; sfx("summon"); }
            else { boss.state = SS_FLICK_OUT; boss.timer = 0.7f; negative_t = 0.7f; sfx("thunder"); }
            if (angry < 1 && !boss.summoned) { boss.summoned = 1; spawn(E_BOMB, (int)((ARENA_X + 160) / TILE), 12); spawn(E_BOMB, (int)((ARENA_X + SCREEN_W - 160) / TILE), 12); sfx("summon"); }
        }
        break;
    case SS_SNIP_AIM:   /* giant scissor blades open around you, then SNIP */
        boss.pose = 1;
        if (boss.timer > 0.25f && t) boss.targetx += (t->x - boss.targetx) * 0.1f;
        if (boss.timer <= 0) { boss.state = SS_SNIP; boss.timer = 0.25f; sfx("slam"); shake = 10; hitstop = 2;
            spark_burst(boss.targetx, FLOOR_Y - 80, 24, 255, 255, 255); ring(boss.targetx, FLOOR_Y - 80, 130, 255, 255, 255); }
        break;
    case SS_SNIP:
        boss.pose = 1; boss_hit_player_box(boss.targetx, FLOOR_Y, 150, 220);
        if (boss.timer <= 0) { if (--boss.slams > 0) { boss.state = SS_SNIP_AIM; boss.timer = 0.5f * angry; } else { boss.state = BS_IDLE; boss.timer = 1.2f * angry; } }
        break;
    case SS_FILM:       /* spits strips of film at you */
        boss.pose = 1;
        if (boss.timer <= 0 && t) {
            float x0 = boss.x + boss.face * 110, y0 = boss.y - 200, dx = t->x - x0, dy = (t->y - 60) - y0, l = sqrtf(dx * dx + dy * dy) + 1;
            Proj *q = proj(P_FILM, x0, y0, dx / l * 520, dy / l * 520, 1, 1); if (q) { q->r = 14; q->life = 3; }
            sfx("spit"); boss.timer = 0.35f * angry; if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.1f * angry; } }
        break;
    case SS_REELS:      /* film reels rain from the rafters and roll along the floor */
        boss.pose = 0;
        if (boss.timer <= 0) { float x = t ? t->x + (frand() - 0.5f) * 320 : ARENA_X + frand() * SCREEN_W;
            Proj *q = proj(P_REEL, clampf(x, ARENA_X + 80, ARENA_X + SCREEN_W - 80), -40, (frand() < 0.5f ? -1 : 1) * (200 + frand() * 120), 0, 1, 1);
            if (q) { q->grav = 1500; q->r = 28; q->bounces = 4; q->life = 5; }
            sfx("clank"); boss.timer = 0.45f * angry; if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.2f * angry; } }
        break;
    case SS_FLICK_OUT:  /* the picture skips: she flickers out and reappears on your other side */
        boss.pose = 0;
        if (boss.timer <= 0) { float side = t ? t->x : boss.x;
            boss.x = side < ARENA_X + SCREEN_W / 2 ? hi : lo; boss.state = SS_FLICK_IN; boss.timer = 0.45f; negative_t = 0.3f; sfx("thunder");
            if (t) boss.face = t->x < boss.x ? -1 : 1; }
        break;
    case SS_FLICK_IN:
        boss.pose = 1;
        if (boss.timer <= 0) { boss.state = SS_SNIP_AIM; boss.slams = 1; boss.timer = 0.55f * angry; boss.targetx = t ? t->x : boss.x; sfx("charge"); }
        break;
    case BS_DEAD:
        if (boss.pose >= 0) boss.pose = 2;
        if (frame_no % 4 == 0) { sfx("boom"); ink_burst(boss.x + (frand() - 0.5f) * 260, boss.y - frand() * 280, 10, 300);
            for (int k = 0; k < 5; k++) part(boss.x, boss.y - 180, (frand() - 0.5f) * 700, -300 - frand() * 400, 1.0f, 5, 800, 200, 196, 186, 0);
            if (frame_no % 8 == 0) gore_part(rand() % 12, boss.x + (frand() - 0.5f) * 200, boss.y - 120 - frand() * 200, 700, rand() & 1); }
        if (!boss.summoned && boss.timer < 1.0f) {
            boss.summoned = 1; shatter(&A.splicer[2], boss.x, boss.y + 10, boss.face > 0, 5, 4, 850, 1); boss.pose = -1;
            for (int k = 0; k < 6; k++) gore_part(11, boss.x, boss.y - 160, 800, 1);
            blood_spurt(boss.x, boss.y - 200, -1, 50, 900); blood_spurt(boss.x, boss.y - 200, 1, 50, 900);
            for (int i = 0; i < 40; i++) { float a = frand() * 6.283f, v = 300 + frand() * 500; part_k(PK_BLOOD, boss.x, boss.y - 180, cosf(a) * v, sinf(a) * v - 300, 1.5f, 5 + frand() * 8, 1500, 12, 8, 12, 0); }
            ring(boss.x, boss.y - 180, 320, 255, 255, 255); shake = 24; hitstop = 10; rumble_all(1.0f, 650); negative_t = 0.25f;
        }
        if (boss.timer <= 0 && scene == SC_PLAY) show_cleared();
        break;
    }
    if (boss.state != BS_DEAD && boss.state != BS_INTRO && boss.state != SS_FLICK_OUT) boss_hit_player_box(boss.x, boss.y, 230, 220);
}
static void draw_boss_splicer(float ox, float oy) {
    float x = sx(boss.x) + ox, y = boss.y + oy + boss.bob;
    if (boss.state == SS_SNIP_AIM || boss.state == SS_SNIP) {   /* the scissor blades closing on you */
        float tx = sx(boss.targetx) + ox, k = boss.state == SS_SNIP ? 1 : clampf(1 - boss.timer / 0.75f, 0, 1);
        float open = boss.state == SS_SNIP ? 0.05f : 0.9f - 0.6f * k;
        for (int sd = -1; sd <= 1; sd += 2) {
            float a = -1.5708f + sd * open, bx = tx, by = FLOOR_Y + oy - 20, ex = bx + cosf(a) * 330, ey = by + sinf(a) * 330;
            draw_bar(bx, by, ex, ey, 26, 20, 18, 18, (Uint8)(150 + 105 * k));
            draw_bar(bx, by, ex, ey, 18, 200, 198, 192, (Uint8)(150 + 105 * k));
            draw_bar(bx + sd * 4, by, ex + sd * 4, ey, 4, 250, 250, 246, 255);
        }
        draw_ellipse(&A.disc, tx, FLOOR_Y + oy - 20, 14, 14, 20, 18, 18, 255);
        if (boss.state == SS_SNIP_AIM && (frame_no / 4) % 2) draw_text_col(&A.small, "!", tx, FLOOR_Y + oy - 380, 0.8f, 1, 255, 60, 40, 255);
    }
    if (boss.pose < 0) return;
    if (boss.state == SS_FLICK_OUT && (frame_no / 2) % 3 != 0) return;   /* skipping frames */
    Tex *t = &A.splicer[boss.pose];
    double ang = boss.state == BS_DEAD ? sinf(frame_no * 0.5f) * 6 : sinf(boss.t * 1.4f) * 2;
    Uint8 al = boss.state == SS_FLICK_OUT || boss.state == SS_FLICK_IN ? 170 : 255;
    draw_sprite(t, x, y + 10, 1, boss.face > 0, boss.flash ? 170 : 0, al, ang, 1.0f + 0.02f * sinf(boss.t * 5));
    if (boss.state == SS_FILM || boss.state == SS_FLICK_IN) draw_glow(&A.disc, x + boss.face * 30, y - 210, 50 + sinf(frame_no * 0.6f) * 10, 255, 250, 230, 220);   /* the projector eye */
}
/* ------------------------------------------------------------ boss: The Big Press (Show 4) */
static void update_boss_press(void) {
    float angry = boss.hp < boss.maxhp / 2 ? 0.7f : 1.0f;
    boss.t += DT; if (boss.flash > 0) boss.flash--;
    boss.timer -= DT; boss.bob = fabsf(sinf(boss.t * 4)) * -4;
    Player *t = nearest_player(boss.x, boss.y, NULL);
    float lo = ARENA_X + 260, hi = ARENA_X + SCREEN_W - 230;
    if (t && boss.state != BS_DEAD) boss.face = t->x < boss.x ? -1 : 1;
    if (boss.state != BS_DEAD && frame_no % 7 == 0) smoke(boss.x - boss.face * 120, boss.y - 300, 1, 160);
    switch (boss.state) {
    case BS_INTRO: boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f; } break;
    case BS_IDLE:
        boss.pose = angry < 1 ? 2 : 0;
        if (t) { float want = t->x - boss.face * 400; boss.x += clampf(want - boss.x, -1, 1) * 70 / angry * DT; }
        boss.x = clampf(boss.x, lo, hi);
        if (frame_no % 30 == 0) { sfx("clank"); shake = fmaxf(shake, 2); }
        if (boss.timer <= 0) {
            int k = rand() % 3;
            if (k == 0) { boss.state = PS_STAMP_AIM; boss.slams = angry < 1 ? 3 : 2; boss.timer = 0.85f * angry; boss.targetx = t ? t->x : boss.x; sfx("charge"); }
            else if (k == 1) { boss.state = PS_SHEETS; boss.slams = angry < 1 ? 6 : 4; boss.timer = 0.35f; }
            else { boss.state = PS_ROLL_WARN; boss.timer = 0.8f * angry; sfx("charge"); }
            if (angry < 1 && !boss.summoned) { boss.summoned = 1; spawn(E_IMP, (int)((ARENA_X + 160) / TILE), 12); spawn(E_IMP, (int)((ARENA_X + 520) / TILE), 12); sfx("summon"); }
        }
        break;
    case PS_STAMP_AIM:  /* a giant printing plate hovers over you, then STAMPS */
        boss.pose = 1;
        if (boss.timer > 0.3f && t) boss.targetx += (t->x - boss.targetx) * 0.09f;
        if (boss.timer <= 0) { boss.state = PS_STAMP; boss.timer = 0.35f; shake = 16; hitstop = 3; sfx("slam"); sfx("boom");
            ring(boss.targetx, FLOOR_Y - 10, 170, 255, 220, 160); smoke(boss.targetx, FLOOR_Y - 10, 6, 60);
            for (int k = 0; k < 20; k++) part_k(PK_BLOOD, boss.targetx + (frand() - 0.5f) * 140, FLOOR_Y - 6, (frand() - 0.5f) * 600, -200 - frand() * 400, 1.2f, 5 + frand() * 5, 1500, 12, 8, 12, 0);
            splat(boss.targetx, FLOOR_Y + 2, 60, 1, 10, 8, 10); }
        break;
    case PS_STAMP:
        boss.pose = 1; boss_hit_player_box(boss.targetx, FLOOR_Y, 160, FLOOR_Y);
        if (boss.timer <= 0) { if (--boss.slams > 0) { boss.state = PS_STAMP_AIM; boss.timer = 0.6f * angry; } else { boss.state = BS_IDLE; boss.timer = 1.3f * angry; } }
        break;
    case PS_SHEETS:     /* spits razor-edged printed sheets that flutter at you */
        boss.pose = 1;
        if (boss.timer <= 0 && t) { float x0 = boss.x + boss.face * 120, y0 = boss.y - 210 + (frand() - 0.5f) * 60;
            Proj *q = proj(P_SHEET, x0, y0, boss.face * (380 + frand() * 140), 0, 1, 1); if (q) { q->r = 16; q->life = 3.5f; }
            sfx("throw"); boss.timer = 0.3f * angry; if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.1f * angry; } }
        break;
    case PS_ROLL_WARN: boss.pose = 1; if ((frame_no / 5) % 2 == 0) smoke(boss.x + boss.face * 120, FLOOR_Y - 20, 1, 30);
        if (boss.timer <= 0) { boss.state = PS_ROLL; boss.sweepx = boss.x + boss.face * 140; sfx("splash"); shake = 8; } break;
    case PS_ROLL:       /* the ink roller rolls out across the floor: jump it */
        boss.pose = 0; boss.sweepx += boss.face * 640 / angry * DT;
        boss_hit_player_box(boss.sweepx, FLOOR_Y, 90, 70);
        if (frame_no % 2 == 0) part_k(PK_BLOOD, boss.sweepx, FLOOR_Y - 30, (frand() - 0.5f) * 200, -200 - frand() * 200, 0.8f, 6, 1200, 12, 8, 12, 0);
        if (frame_no % 4 == 0) splat(boss.sweepx, FLOOR_Y + 2, 26, 1, 10, 8, 10);
        if (boss.sweepx < ARENA_X - 60 || boss.sweepx > ARENA_X + SCREEN_W + 60) { boss.state = BS_IDLE; boss.timer = 1.1f * angry; }
        break;
    case BS_DEAD:
        if (boss.pose >= 0) boss.pose = 2;
        if (frame_no % 4 == 0) { sfx("boom"); spark_burst(boss.x + (frand() - 0.5f) * 260, boss.y - frand() * 300, 14, 255, 200, 110); smoke(boss.x, boss.y - 200, 2, 70);
            ink_burst(boss.x + (frand() - 0.5f) * 220, boss.y - frand() * 260, 10, 300);
            if (frame_no % 8 == 0) gore_part(rand() % 8, boss.x + (frand() - 0.5f) * 200, boss.y - 120 - frand() * 200, 700, 0); }
        if (!boss.summoned && boss.timer < 1.0f) {
            boss.summoned = 1; shatter(&A.press[2], boss.x, boss.y + 10, boss.face > 0, 5, 4, 850, 1); boss.pose = -1;
            for (int k = 0; k < 14; k++) part_k(PK_DEBRIS, boss.x, boss.y - 180, (frand() - 0.5f) * 1000, -300 - frand() * 600, 2.5f, 8 + frand() * 10, 1600, 60, 52, 46, 0);
            for (int i = 0; i < 50; i++) { float a = frand() * 6.283f, v = 300 + frand() * 600; part_k(PK_BLOOD, boss.x, boss.y - 180, cosf(a) * v, sinf(a) * v - 300, 1.5f, 5 + frand() * 8, 1500, 12, 8, 12, 0); }
            for (int i = 0; i < 12; i++) { Proj *q = proj(P_SHEET, boss.x, boss.y - 200, (frand() - 0.5f) * 700, 0, 0, 0); if (q) { q->life = 1.5f; q->enemy = 0; q->dmg = 0; } }
            ring(boss.x, boss.y - 180, 320, 255, 220, 160); shake = 24; hitstop = 10; rumble_all(1.0f, 650);
        }
        if (boss.timer <= 0 && scene == SC_PLAY) show_cleared();
        break;
    }
    if (boss.state != BS_DEAD && boss.state != BS_INTRO) boss_hit_player_box(boss.x, boss.y, 240, 220);
}
static void draw_boss_press(float ox, float oy) {
    float x = sx(boss.x) + ox, y = boss.y + oy + boss.bob;
    if (boss.state == PS_STAMP_AIM || boss.state == PS_STAMP) {   /* the printing plate: a slab of iron type */
        float tx = sx(boss.targetx) + ox, k = boss.state == PS_STAMP ? 1 : clampf(1 - boss.timer / 0.85f, 0, 1);
        float bot = boss.state == PS_STAMP ? FLOOR_Y + oy : 70 + oy + sinf(frame_no * 0.8f) * 3;
        if (boss.state == PS_STAMP_AIM) { draw_ellipse(&A.disc, tx, FLOOR_Y + oy, 70 + 30 * k, 10 + 5 * k, 0, 0, 0, (Uint8)(110 + 130 * k));
            if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", tx, FLOOR_Y + oy - 80, 0.8f, 1, 255, 60, 40, 255); }
        draw_bar(tx, 0, tx, bot - 60, 16, 50, 44, 40, 255);
        fill_rect((int)tx - 84, (int)bot - 64, 168, 64, 24, 20, 18, 255); fill_rect((int)tx - 80, (int)bot - 60, 160, 56, 90, 78, 64, 255);
        fill_rect((int)tx - 80, (int)bot - 60, 160, 6, 190, 150, 70, 255);
        for (int r = 0; r < 2; r++) for (int c = 0; c < 6; c++) fill_rect((int)tx - 72 + c * 25, (int)bot - 44 + r * 18, 18, 12, 30, 26, 22, 255);
    }
    if (boss.state == PS_ROLL) { float bx = sx(boss.sweepx) + ox;   /* the ink roller */
        draw_ellipse(&A.disc, bx, FLOOR_Y + oy - 36, 40, 40, 10, 8, 10, 255); draw_ellipse(&A.disc, bx - 10, FLOOR_Y + oy - 48, 10, 6, 120, 120, 130, 160);
        draw_bar(bx, FLOOR_Y + oy - 36, bx - boss.face * 60, FLOOR_Y + oy - 6, 18, 10, 8, 10, 255); }
    if (boss.pose < 0) return;
    Tex *t = &A.press[boss.pose];
    double ang = boss.state == BS_DEAD ? sinf(frame_no * 0.5f) * 5 : 0;
    draw_sprite(t, x, y + 10, 1, boss.face > 0, boss.flash ? 170 : 0, 255, ang, 1.0f + 0.015f * sinf(boss.t * 8));
}
/* ------------------------------------------------------------ boss: The Collector (Show 5) */
static void update_boss_collector(void) {
    float angry = boss.hp < boss.maxhp / 2 ? 0.7f : 1.0f;
    boss.t += DT; if (boss.flash > 0) boss.flash--;
    boss.timer -= DT; boss.bob = sinf(boss.t * 2.4f) * 8;
    Player *t = nearest_player(boss.x, boss.y, NULL);
    float lo = ARENA_X + 250, hi = ARENA_X + SCREEN_W - 220;
    if (t && boss.state != BS_DEAD) boss.face = t->x < boss.x ? -1 : 1;
    switch (boss.state) {
    case BS_INTRO: boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f; } break;
    case BS_IDLE:
        boss.pose = angry < 1 ? 2 : 0;
        if (t) { float want = t->x - boss.face * 380; boss.x += clampf(want - boss.x, -1, 1) * 120 / angry * DT; }
        boss.x = clampf(boss.x, lo, hi);
        if (boss.timer <= 0) {
            int k = rand() % 3;
            if (k == 0) { boss.state = CS_STAB_AIM; boss.slams = angry < 1 ? 3 : 2; boss.timer = 0.75f * angry; boss.targetx = t ? t->x : boss.x; sfx("charge"); }
            else if (k == 1) { boss.state = CS_THROW; boss.slams = angry < 1 ? 6 : 4; boss.timer = 0.3f; }
            else { boss.state = CS_SUCK; boss.timer = 2.4f; sfx("roar1"); message("HE'S TRYING TO BAG YOU! RUN!", 1.4f); }
            if (angry < 1 && !boss.summoned) { boss.summoned = 1; spawn(E_SOLDIER, (int)((ARENA_X + 140) / TILE), 12); spawn(E_SOLDIER, (int)((ARENA_X + 480) / TILE), 12); sfx("summon"); }
        }
        break;
    case CS_STAB_AIM:   /* a giant umbrella spike hovers over you, then plunges */
        boss.pose = 1;
        if (boss.timer > 0.25f && t) boss.targetx += (t->x - boss.targetx) * 0.1f;
        if (boss.timer <= 0) { boss.state = CS_STAB; boss.timer = 0.3f; shake = 12; hitstop = 2; sfx("slam");
            ring(boss.targetx, FLOOR_Y - 10, 120, 255, 240, 200); spark_burst(boss.targetx, FLOOR_Y - 8, 16, 255, 240, 200);
            for (int k = 0; k < 6; k++) part_k(PK_DEBRIS, boss.targetx, FLOOR_Y - 6, (frand() - 0.5f) * 500, -200 - frand() * 300, 1.5f, 6, 1600, 110, 80, 50, 0); }
        break;
    case CS_STAB:
        boss.pose = 1; boss_hit_player_box(boss.targetx, FLOOR_Y, 90, FLOOR_Y);
        if (boss.timer <= 0) { if (--boss.slams > 0) { boss.state = CS_STAB_AIM; boss.timer = 0.5f * angry; } else { boss.state = BS_IDLE; boss.timer = 1.2f * angry; } }
        break;
    case CS_THROW:      /* pulls lost shoes out of the sack and hurls them */
        boss.pose = 1;
        if (boss.timer <= 0 && t) { float x0 = boss.x - boss.face * 40, y0 = boss.y - 330, dx = t->x - x0;
            Proj *q = proj(P_SHOE, x0, y0, clampf(dx / 1.0f, -560, 560) + (frand() - 0.5f) * 120, -620, 1, 1); if (q) { q->grav = 1300; q->r = 16; q->bounces = 2; q->life = 4; }
            sfx("throw"); boss.timer = 0.32f * angry; if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.1f * angry; } }
        break;
    case CS_SUCK: {     /* opens the sack: everything gets dragged toward it. Run the other way */
        boss.pose = angry < 1 ? 2 : 0;
        float mx = boss.x - boss.face * 30, my = boss.y - 200;
        for (int i = 0; i < 2; i++) { float a = frand() * 6.283f, r = 300 + frand() * 200; part(mx + cosf(a) * r, my + sinf(a) * r * 0.6f, -cosf(a) * r * 2.2f, -sinf(a) * r * 1.3f, 0.45f, 4, 0, 220, 210, 190, 0); }
        if (frame_no % 10 == 0) sfx("summon");
        for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (!p->active || !p->alive || p->hidden) continue;
            float dir = boss.x < p->x ? -1 : 1, pull = 260 / angry * DT, vy0 = 0;
            move_box(&p->x, &p->y, &(float){dir * pull / DT}, &vy0, 40, 120, 0); }
        if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f * angry; }
        break; }
    case BS_DEAD:
        if (boss.pose >= 0) boss.pose = 2;
        if (frame_no % 4 == 0) { sfx("boom"); blood_spurt(boss.x + (frand() - 0.5f) * 160, boss.y - frand() * 300, frand() < 0.5f ? -1 : 1, 10, 700);
            for (int k = 0; k < 3; k++) { Proj *q = proj(P_SHOE, boss.x, boss.y - 300, (frand() - 0.5f) * 700, -500 - frand() * 300, 0, 0); if (q) { q->grav = 1300; q->bounces = 2; q->life = 2; } }
            if (frame_no % 8 == 0) gore_part(rand() % 12, boss.x + (frand() - 0.5f) * 200, boss.y - 120 - frand() * 200, 700, 1); }
        if (!boss.summoned && boss.timer < 1.0f) {
            boss.summoned = 1; shatter(&A.collector[2], boss.x, boss.y + 10, boss.face > 0, 4, 5, 850, 1); boss.pose = -1;
            for (int k = 0; k < 6; k++) gore_part(11, boss.x, boss.y - 200, 800, 1);
            gore_part(3, boss.x - 20, boss.y - 330, 900, 1); gore_part(3, boss.x + 20, boss.y - 330, 900, 1);
            blood_spurt(boss.x, boss.y - 250, -1, 60, 900); blood_spurt(boss.x, boss.y - 250, 1, 50, 800);
            for (int i = 0; i < 20; i++) part_k(PK_DEBRIS, boss.x, boss.y - 250, (frand() - 0.5f) * 1000, -300 - frand() * 600, 2.5f, 6 + frand() * 10, 1600, 120 + rand() % 120, 80 + rand() % 100, 50 + rand() % 80, 0);
            ring(boss.x, boss.y - 200, 300, 255, 220, 160); shake = 24; hitstop = 10; rumble_all(1.0f, 650);
        }
        if (boss.timer <= 0 && scene == SC_PLAY) show_cleared();
        break;
    }
    if (boss.state != BS_DEAD && boss.state != BS_INTRO) boss_hit_player_box(boss.x, boss.y, 160, 300);
}
static void draw_boss_collector(float ox, float oy) {
    float x = sx(boss.x) + ox, y = boss.y + oy + boss.bob;
    if (boss.state == CS_STAB_AIM || boss.state == CS_STAB) {   /* a giant black umbrella spike */
        float tx = sx(boss.targetx) + ox, k = boss.state == CS_STAB ? 1 : clampf(1 - boss.timer / 0.75f, 0, 1);
        float tip = boss.state == CS_STAB ? FLOOR_Y + oy : 120 + oy + sinf(frame_no * 0.9f) * 4;
        if (boss.state == CS_STAB_AIM) { draw_ellipse(&A.disc, tx, FLOOR_Y + oy, 30 + 24 * k, 7 + 4 * k, 0, 0, 0, (Uint8)(110 + 130 * k));
            if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", tx, FLOOR_Y + oy - 80, 0.8f, 1, 255, 60, 40, 255); }
        draw_bar(tx, 0, tx, tip - 140, 10, 90, 60, 30, 255);   /* handle */
        for (int i = 0; i < 6; i++) { float w = 70 - i * 11; fill_rect((int)(tx - w), (int)(tip - 140 + i * 20), (int)(w * 2), 22, 14, 12, 16, 255); }   /* folded canopy */
        draw_bar(tx, tip - 30, tx, tip, 6, 200, 200, 210, 255);   /* metal ferrule tip */
    }
    if (boss.pose < 0) return;
    Tex *t = &A.collector[boss.pose];
    double ang = boss.state == BS_DEAD ? sinf(frame_no * 0.5f) * 6 : sinf(boss.t * 1.2f) * 3;
    draw_sprite(t, x, y + 10, 1, boss.face > 0, boss.flash ? 170 : 0, 255, ang, 1.0f);
    if (boss.state == CS_SUCK) draw_glow(&A.disc, x - boss.face * 40, y - 220, 90 + sinf(frame_no * 0.5f) * 15, 40, 30, 50, 180);
}
/* ============================================================ SHOW 6: HALL OF MIRRORS / SHOW 7: ROOFTOP MARQUEE */

/* ---- mirror panes ('Y'): solid glass that bounces every shot back the way it came (yours too).
   Each hit cracks it; five and it shatters. The welding torch melts glass instead of bouncing off it. */
static int mirror_hp[LW];
static void mirror_shatter(int tx) {
    for (int y = 0; y < LH; y++) if (map[y][tx] == 'Y') {
        map[y][tx] = '.';
        for (int k = 0; k < 7; k++) part_k(PK_DEBRIS, tx * TILE + 24, y * TILE + 24, (frand() - 0.5f) * 700, -150 - frand() * 400, 1.6f, 4 + frand() * 6, 1600, 190, 215, 235, 0);
        spark_burst(tx * TILE + 24, y * TILE + 24, 6, 230, 245, 255);
    }
    sfx("shatter"); if (shake < 7) shake = 7; score += 150; hitstop = 2;
}
static int mirror_hit(Proj *p, int tx) {   /* 1 = the projectile was handled here */
    mirror_hp[tx]--;
    spark_burst(p->x, p->y, 5, 220, 240, 255);
    if (mirror_hp[tx] <= 0) { mirror_shatter(tx); p->alive = 0; return 1; }
    if (p->type == P_FLAME || fabsf(p->vx) < 60) { if (p->type != P_FLAME) sfx("clank"); p->alive = 0; return 1; }
    p->x = p->vx > 0 ? tx * TILE - 3 : (tx + 1) * TILE + 3;
    p->vx = -p->vx * 0.8f; p->enemy = !p->enemy; p->last_hit = -1; p->dmg = 1;
    if (p->life < 0.6f) p->life = 0.6f;
    sfx("clank"); return 1;
}
static void mirror(int tx, int y0, int y1) { mark_busy(tx - 1, tx + 1); tx = RX(tx); for (int y = y0; y <= y1; y++) put_raw(tx, y, 'Y'); if (tx >= 0 && tx < LW) mirror_hp[tx] = 5; }
/* a reflection of the runner, living on the far side of an invisible mirror line */
static void reflection(int tx, int line_tx) {
    mark_span(tx, line_tx);   /* a gap between a reflection and its mirror line would move the mirror */
    int n0 = nen; spawn(E_MIRROR, tx, 13); if (nen == n0) return;
    tx = RX(tx); line_tx = RX(line_tx);
    Enemy *e = &en[nen - 1]; e->mx = line_tx * TILE + 24; e->plat0 = tx > line_tx ? 1 : -1;
}

/* ---- the roof: neon letter platforms ('n') flicker on and off, lightning rods ('I') */
static float neon_phase(int tx) { return fmodf(scene_t + (tx / 4) * 0.9f, 3.6f); }
static int neon_on(int tx) { return neon_phase(tx) < 2.5f; }
static void neon(int x0, int x1, int y) { mark_busy(x0, x1); x0 = RX(x0); x1 = RX(x1); for (int x = x0; x <= x1; x++) put_raw(x, y, 'n'); }
static void rod(int tx, int y) { mark_busy(tx - 1, tx + 1); put(tx, y, 'I'); }

/* ---- the storm: wind gusts, rain, and lightning that strikes wherever the lights are on */
#define NBOLT 6
static struct { float x, t; int state; } bolts[NBOLT];   /* state 0 free, 1 warning, 2 striking */
static float storm_t = 3.0f, flash_t, wind_push, wind_clock;
static void bolt_add(float x, float warn) {
    for (int i = 0; i < NBOLT; i++) if (!bolts[i].state) { bolts[i].x = x; bolts[i].t = warn; bolts[i].state = 1; sfx("charge"); return; }
}
static void bolt_strike(float x) {
    sfx("thunder"); shake = fmaxf(shake, 12); flash_t = 0.22f; hitstop = 2;
    ring(x, FLOOR_Y - 10, 140, 255, 255, 220); spark_burst(x, FLOOR_Y - 10, 24, 255, 250, 200); smoke(x, FLOOR_Y - 10, 4, 70);
    splat(x, FLOOR_Y + 2, 40, 1, 20, 16, 14);
    for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (p->active && p->alive && !p->hidden && fabsf(p->x - x) < 50) hurt_player(p, x); }
    for (int i = 0; i < nen; i++) { Enemy *e = &en[i]; if (e->alive && e->type < E_CRATE && fabsf(e->x - x) < 80) { e->flash = 6; enemy_damage(e, 99); } }
    if (boss.active && boss.state != BS_DEAD && boss.state != BS_INTRO && fabsf(boss.x - x) < 130) {
        boss.hp = boss.hp > 9 ? boss.hp - 8 : 1; boss.flash = 8; message("LIGHTNING ROD! HE TOOK THE HIT!", 1.2f); }
}
static void update_storm(void) {
    if (level_no != L_ROOF) return;
    if (flash_t > 0) flash_t -= DT;
    /* wind: every nine seconds a gust shoves everyone back the way they came */
    wind_clock += DT; float ph = fmodf(wind_clock, 9.0f); wind_push = 0;
    if (!boss.active && ph > 6.4f && ph < 8.6f) {
        wind_push = -230 * fminf(1, (ph - 6.4f) * 3) * fminf(1, (8.6f - ph) * 3);
        if (ph < 6.4f + DT * 1.5f) { message("GUST OF WIND! HOLD ON!", 1.2f); sfx("summon"); }
        for (int k = 0; k < 2; k++) { Player *p = &pl[k]; if (!p->active || !p->alive || p->hidden) continue;
            float wv = wind_push * (p->crouch ? 0.35f : 1.0f), vy0 = 0; move_box(&p->x, &p->y, &wv, &vy0, 40, 120, 0); }
    }
    /* lightning: the GamePad's LIGHTS draw it. A lit zone with you in it is a bad place to stand */
    storm_t -= DT;
    if (storm_t <= 0) {
        storm_t = (loop_no ? 2.4f : 3.0f) + frand() * 1.8f;
        float x = -1;
        Player *t = nearest_player(camx + SCREEN_W / 2, FLOOR_Y, NULL);
        if (boss.active) { if (lit[zone_of(boss.x)] > 0) x = boss.x + (frand() - 0.5f) * 60; }
        else if (t && lit[zone_of(t->x)] > 0) x = t->x + (frand() - 0.5f) * 180;
        else {
            for (int i = 0; i < nen && x < 0; i++) { Enemy *e = &en[i];
                if (e->alive && e->type < E_CRATE && e->x > camx && e->x < camx + SCREEN_W && lit[zone_of(e->x)] > 0) x = e->x; }
            if (x < 0) { int rods[24], nr = 0, t0 = (int)(camx / TILE), t1 = t0 + SCREEN_W / TILE;
                for (int tx = t0; tx <= t1 && nr < 24; tx++) for (int ty = 0; ty < LH; ty++) if (tx >= 0 && tx < LW && map[ty][tx] == 'I') { rods[nr++] = tx; break; }
                x = nr ? rods[rand() % nr] * TILE + 24 : camx + 120 + frand() * (SCREEN_W - 240); }
        }
        if (x >= 0 && !(boss.active && x < 0)) bolt_add(x, loop_no ? 0.75f : 0.9f);
    }
    for (int i = 0; i < NBOLT; i++) {
        if (!bolts[i].state) continue;
        bolts[i].t -= DT;
        if (bolts[i].state == 1) {
            if (frame_no % 3 == 0) part(bolts[i].x + (frand() - 0.5f) * 40, FLOOR_Y - 4, (frand() - 0.5f) * 120, -200 - frand() * 200, 0.3f, 3, 600, 255, 250, 200, 1);
            if (bolts[i].t <= 0) { bolts[i].state = 2; bolts[i].t = 0.28f; bolt_strike(bolts[i].x); }
        } else if (bolts[i].t <= 0) bolts[i].state = 0;
    }
}
static void draw_storm_back(float ox, float oy) {   /* warnings sit behind the actors */
    if (level_no != L_ROOF) return;
    for (int i = 0; i < NBOLT; i++) { if (bolts[i].state != 1) continue;
        float x = sx(bolts[i].x) + ox, k = 1 - bolts[i].t / 0.9f;
        fill_rect((int)(x - 20), 0, 40, (int)(FLOOR_Y + oy), 255, 250, 210, (Uint8)(20 + 50 * k));
        draw_ellipse(&A.disc, x, FLOOR_Y + oy, 40 + 20 * k, 8, 255, 240, 160, (Uint8)(120 + 120 * k));
        if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", x, FLOOR_Y + oy - 90, 0.9f, 1, 255, 240, 120, 255);
    }
}
static void draw_storm_front(float ox, float oy) {
    if (level_no != L_ROOF) return;
    for (int i = 0; i < NBOLT; i++) { if (bolts[i].state != 2) continue;
        float px = sx(bolts[i].x) + ox, py = 0; unsigned seed = (unsigned)(bolts[i].x * 7) + (unsigned)(frame_no / 2) * 131u;
        for (int s = 1; s <= 12; s++) {
            seed = seed * 1103515245u + 12345u;
            float nx = sx(bolts[i].x) + ox + (s == 12 ? 0 : (float)((int)(seed >> 16) % 70 - 35)), ny = (FLOOR_Y + oy) * s / 12.0f;
            draw_bar(px, py, nx, ny, 14, 160, 170, 255, 120); draw_bar(px, py, nx, ny, 6, 255, 255, 255, 255);
            px = nx; py = ny;
        }
        draw_glow(&A.disc, sx(bolts[i].x) + ox, FLOOR_Y + oy - 10, 120, 220, 230, 255, 220);
    }
    /* rain, slanting harder in the wind */
    float slant = 0.25f - wind_push * 0.002f;
    for (int i = 0; i < 90; i++) {
        float x = fmodf(i * 137.0f + scene_t * (220 + slant * 900) + camx * -0.2f, SCREEN_W + 200) - 100;
        float y = fmodf(i * 211.0f + scene_t * 1100, SCREEN_H + 60) - 40;
        if (x < -100) x += SCREEN_W + 200;
        draw_bar(x, y, x - slant * 40, y + 34, 2, 170, 190, 220, 110);
    }
    if (flash_t > 0) fill_rect(0, 0, SCREEN_W, SCREEN_H, 240, 245, 255, (Uint8)(flash_t / 0.22f * 150));
    if (wind_push < -40) for (int i = 0; i < 10; i++) { float y = 80 + i * 60 + sinf(scene_t * 3 + i) * 20, x = fmodf(SCREEN_W - scene_t * 1600 - i * 333, SCREEN_W + 400) + 200;
        if (x > SCREEN_W + 200) x -= SCREEN_W + 400; draw_bar(x, y, x + 140, y, 2, 220, 230, 240, (Uint8)(-wind_push * 0.4f)); }
}

/* ---- SHOW 6 layout */
static void build_level7(void) {
    for (int y = 0; y < LH; y++) { memset(map[y], '.', LW); map[y][LW] = 0; }
    memset(mirror_hp, 0, sizeof mirror_hp);
    nen = nbreak = ndrip = ncheck = 0;
    block(0, LW - 1, 13, 14);
    block(0, 0, 0, 12); block(LW - 1, LW - 1, 0, 12);
    /* STAGE DOOR: the first mannequin, and the first mirror */
    checkpoint(2);
    spawn(E_MANNEQUIN, 15, 13); plat(18, 23, 10); spawn(E_CRATE, 20, 10); spawn(E_PUFF, 25, 7); breaker(27);
    mirror(31, 10, 12); spawn(E_USHER, 35, 13);
    /* WARDROBE: costume racks to climb, mannequins among the coats */
    checkpoint(41);
    block(46, 48, 11, 12); spawn(E_MANNEQUIN, 52, 13); spawn(E_MANNEQUIN, 56, 13); plat(55, 61, 9); spawn(E_PUFF, 59, 6);
    mirror(64, 10, 12); spawn(E_MANNEQUIN, 68, 13); block(71, 73, 12, 12); plat(76, 82, 9); spawn(E_HATCRATE, 79, 9);
    /* MAKEUP ROW: powder puffs, and your first reflection */
    checkpoint(86);
    spawn(E_PUFF, 91, 7); spawn(E_MANNEQUIN, 95, 13); reflection(106, 101); spawn(E_PUFF, 108, 6);
    pit(111, 116); plat(110, 113, 10); plat(115, 118, 9); spawn(E_MANNEQUIN, 120, 13); breaker(122);
    /* HALL OF MIRRORS: panes, reflections, puffs */
    checkpoint(126);
    mirror(131, 10, 12); reflection(140, 136); mirror(145, 9, 12); plat(147, 152, 9); spawn(E_PUFF, 150, 6);
    reflection(159, 155); mirror(163, 10, 12); spawn(E_MANNEQUIN, 166, 13); breaker(168);
    /* THE STAR'S DRESSING ROOM: over the ink on make-up tables */
    checkpoint(171);
    pit(175, 196); plat(175, 178, 10); plat(181, 184, 9); spawn(E_PUFF, 183, 5); plat(187, 190, 10); spawn(E_PUFF, 189, 6);
    plat(193, 197, 9); mirror(195, 6, 8); spawn(E_MANNEQUIN, 201, 13); breaker(203); reflection(207, 204);
    /* THE VANITY (boss) */
    checkpoint(209);
    for (int i = 0; i < nen; i++) en[i].plat1 = LW * TILE;
}
/* ---- SHOW 7 layout */
static void build_level8(void) {
    for (int y = 0; y < LH; y++) { memset(map[y], '.', LW); map[y][LW] = 0; }
    memset(bolts, 0, sizeof bolts); storm_t = 3.0f; flash_t = 0; wind_clock = 0;
    nen = nbreak = ndrip = ncheck = 0;
    block(0, LW - 1, 13, 14);
    block(0, 0, 0, 12); block(LW - 1, LW - 1, 0, 12);
    /* FIRE ESCAPE: up the iron stairs into the rain */
    checkpoint(2);
    spawn(E_BULB, 14, 13); plat(17, 21, 10); plat(22, 26, 8); spawn(E_CROW, 24, 5); rod(29, 12); breaker(31);
    spawn(E_UMBRELLA, 33, 7); spawn(E_BULB, 36, 13);
    /* THE ROOFTOP: chimneys, lightning rods, the first neon letters */
    checkpoint(41);
    block(45, 47, 12, 12); rod(46, 11); spawn(E_CROW, 52, 6); spawn(E_BULB, 55, 13); pit(58, 62); neon(57, 63, 10); spawn(E_CROW, 61, 5);
    block(66, 68, 11, 12); rod(67, 10); spawn(E_BULB, 72, 13); rod(74, 12); plat(76, 82, 9); spawn(E_HATCRATE, 79, 9);
    /* WATER TOWERS */
    checkpoint(86);
    block(90, 93, 10, 12); rod(91, 9); spawn(E_CROW, 95, 5); pit(97, 103); neon(96, 104, 10); spawn(E_UMBRELLA, 100, 6);
    block(106, 108, 11, 12); spawn(E_BULB, 107, 10); spawn(E_CROW, 112, 4); pit(114, 118); plat(113, 119, 9); spawn(E_BULB, 121, 13); breaker(123);
    /* THE NEON SIGN: across the giant letters while they flicker */
    checkpoint(126);
    pit(130, 165); neon(129, 133, 10); plat(135, 137, 9); spawn(E_CROW, 137, 5); neon(139, 143, 10); neon(145, 149, 9);
    spawn(E_CROW, 149, 4); plat(151, 153, 10); neon(155, 159, 9); spawn(E_CROW, 158, 5); neon(161, 166, 10); breaker(168);
    /* LIGHTNING RODS: the strikes come thick and fast */
    checkpoint(171);
    rod(176, 12); spawn(E_BULB, 180, 13); rod(184, 12); spawn(E_CROW, 186, 5); block(188, 190, 11, 12); rod(189, 10);
    spawn(E_BULB, 194, 13); spawn(E_UMBRELLA, 197, 6); rod(200, 12); breaker(203); spawn(E_CROW, 205, 5);
    /* THE MARQUEE (boss) */
    checkpoint(209);
    for (int i = 0; i < nen; i++) en[i].plat1 = LW * TILE;
}

/* ------------------------------------------------------------ boss: Madame Vanity (Show 6) */
enum { VS_LIPSTICK = 120, VS_SPLIT, VS_SHARDS };
static float vfx[3]; static int vreal, vfake_alive[3], vsplit_hp;
static void vanity_end_split(int shatter_fakes) {
    for (int i = 0; i < 3; i++) if (i != vreal && vfake_alive[i]) {
        vfake_alive[i] = 0;
        for (int k = 0; k < 10; k++) part_k(PK_DEBRIS, vfx[i], boss.y - 200, (frand() - 0.5f) * 700, -200 - frand() * 400, 1.5f, 5 + frand() * 6, 1600, 190, 215, 235, 0);
        if (shatter_fakes) spark_burst(vfx[i], boss.y - 200, 16, 230, 245, 255); else smoke(vfx[i], boss.y - 200, 4, 200);
    }
    if (shatter_fakes) sfx("shatter");
}
static int vanity_fake_hit(Proj *p) {   /* shots that hit a reflection shatter it, and the shards fly at you */
    if (level_no != L_MIRROR || boss.state != VS_SPLIT || p->enemy) return 0;
    for (int i = 0; i < 3; i++) if (i != vreal && vfake_alive[i] && overlap(vfx[i], boss.y + boss.bob - 40, 200, 340, p->x, p->y + p->r, p->r * 2, p->r * 2)) {
        vfake_alive[i] = 0; p->alive = 0; sfx("shatter"); message("A FAKE! SEVEN YEARS BAD LUCK FOR HER!", 1.0f);
        for (int k = 0; k < 12; k++) part_k(PK_DEBRIS, vfx[i], boss.y - 200, (frand() - 0.5f) * 800, -200 - frand() * 500, 1.5f, 5 + frand() * 6, 1600, 190, 215, 235, 0);
        Player *t = nearest_player(vfx[i], boss.y, NULL);
        for (int k = -1; k <= 1; k++) if (t) { float dx = t->x - vfx[i], dy = (t->y - 70) - (boss.y - 220), l = sqrtf(dx * dx + dy * dy) + 1;
            Proj *q = proj(P_SHARD, vfx[i], boss.y - 220, dx / l * 420 + k * 90, dy / l * 420 + k * 40, 1, 1); if (q) { q->r = 10; q->life = 2.5f; } }
        return 1;
    }
    return 0;
}
static void update_boss_vanity(void) {
    float angry = boss.hp < boss.maxhp / 2 ? 0.7f : 1.0f;
    boss.t += DT; if (boss.flash > 0) boss.flash--;
    boss.timer -= DT; boss.bob = sinf(boss.t * 2.0f) * 8;
    Player *t = nearest_player(boss.x, boss.y, NULL);
    float lo = ARENA_X + 230, hi = ARENA_X + SCREEN_W - 210;
    if (t && boss.state != BS_DEAD) boss.face = t->x < boss.x ? -1 : 1;
    switch (boss.state) {
    case BS_INTRO: boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f; } break;
    case BS_IDLE:
        boss.pose = angry < 1 ? 2 : 0;
        if (t) { float want = t->x - boss.face * 420; boss.x += clampf(want - boss.x, -1, 1) * 110 / angry * DT; }
        boss.x = clampf(boss.x, lo, hi);
        if (boss.timer <= 0) {
            int k = rand() % 3;
            if (k == 0) { boss.state = VS_LIPSTICK; boss.slams = angry < 1 ? 5 : 3; boss.timer = 0.4f; sfx("charge"); }
            else if (k == 1) { boss.state = VS_SPLIT; boss.timer = 6.0f; boss.slams = 0; vsplit_hp = boss.hp;
                vfx[0] = ARENA_X + 240; vfx[1] = ARENA_X + SCREEN_W / 2; vfx[2] = ARENA_X + SCREEN_W - 230; vreal = rand() % 3;
                for (int i = 0; i < 3; i++) { vfake_alive[i] = i != vreal; ring(vfx[i], boss.y - 200, 160, 230, 240, 255); smoke(vfx[i], boss.y - 200, 4, 220); }
                boss.x = vfx[vreal]; sfx("summon"); message("WHICH ONE IS THE REAL MADAME? (THE FAKES ARE BACKWARDS)", 2.2f); }
            else { boss.state = VS_SHARDS; boss.slams = angry < 1 ? 10 : 7; boss.timer = 0.3f; boss.sweepx = t ? t->x - boss.face * 320 : boss.x; sfx("roar1"); message("MIND THE GLASS!", 1.0f); }
            if (angry < 1 && !boss.summoned) { boss.summoned = 1; int n0 = nen; spawn(E_MIRROR, (int)((ARENA_X + 160) / TILE), 12);
                if (nen > n0) { en[nen - 1].mx = ARENA_X + SCREEN_W / 2; en[nen - 1].plat0 = 0; } sfx("summon"); message("SHE SUMMONS YOUR REFLECTION!", 1.4f); }
        }
        break;
    case VS_LIPSTICK:   /* a giant lipstick fires red rockets that steer after you */
        boss.pose = 1;
        if (boss.timer <= 0 && t) { float x0 = boss.x + boss.face * 100, y0 = boss.y - 250, dx = t->x - x0, dy = (t->y - 70) - y0, l = sqrtf(dx * dx + dy * dy) + 1;
            Proj *q = proj(P_LIPSTICK, x0, y0, dx / l * 360, dy / l * 360, 1, 1); if (q) { q->r = 12; q->life = 3.6f; }
            sfx("throw"); boss.timer = 0.5f * angry; if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.2f * angry; } }
        break;
    case VS_SPLIT:      /* three of her blow kisses at you; only one is real */
        boss.pose = 1;
        if (boss.hp < vsplit_hp) { vanity_end_split(1); message("THAT'S THE REAL ONE!", 1.0f); boss.state = BS_IDLE; boss.timer = 1.4f * angry; break; }
        if (fmodf(boss.timer, 1.1f * angry) < DT && t) for (int i = 0; i < 3; i++) if (i == vreal || vfake_alive[i]) {
            float x0 = vfx[i] - boss.face * 30, y0 = boss.y - 240, dx = t->x - x0, dy = (t->y - 70) - y0, l = sqrtf(dx * dx + dy * dy) + 1;
            Proj *q = proj(P_KISS, x0, y0, dx / l * 260, dy / l * 260, 1, 1); if (q) { q->r = 13; q->life = 4.0f; } }
        if (boss.timer <= 0) { vanity_end_split(0); boss.state = BS_IDLE; boss.timer = 1.0f * angry; }
        break;
    case VS_SHARDS:     /* the ceiling mirrors crack and fall in a line that sweeps across the room */
        boss.pose = angry < 1 ? 2 : 1;
        if (boss.timer <= 0) { float x = clampf(boss.sweepx, ARENA_X + 40, ARENA_X + SCREEN_W - 40);
            Proj *q = proj(P_SHARD, x, 40, 0, 0, 1, 1); if (q) { q->r = 12; q->life = 3.0f; q->bounces = 1; }
            boss.sweepx += boss.face * 120; boss.timer = 0.24f * angry; sfx("tick");
            if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.3f * angry; } }
        break;
    case BS_DEAD:
        if (boss.pose >= 0) boss.pose = 2;
        if (frame_no % 4 == 0) { sfx("shatter"); spark_burst(boss.x + (frand() - 0.5f) * 160, boss.y - frand() * 300, 8, 230, 245, 255);
            for (int k = 0; k < 4; k++) part_k(PK_DEBRIS, boss.x, boss.y - 250, (frand() - 0.5f) * 700, -300 - frand() * 400, 1.6f, 5 + frand() * 6, 1600, 190, 215, 235, 0);
            if (frame_no % 8 == 0) blood_spurt(boss.x, boss.y - 200, frand() < 0.5f ? -1 : 1, 10, 600); }
        if (!boss.summoned && boss.timer < 1.0f) {
            boss.summoned = 1; shatter(&A.vanity[2], boss.x, boss.y + 10, boss.face > 0, 4, 5, 850, 1); boss.pose = -1;
            for (int i = 0; i < 40; i++) part_k(PK_DEBRIS, boss.x, boss.y - 260, (frand() - 0.5f) * 1100, -300 - frand() * 700, 2.5f, 5 + frand() * 10, 1600, 180 + rand() % 60, 200 + rand() % 50, 230, 0);
            for (int k = 0; k < 6; k++) gore_part(11, boss.x, boss.y - 200, 800, 1);
            blood_spurt(boss.x, boss.y - 250, -1, 50, 900); blood_spurt(boss.x, boss.y - 250, 1, 40, 800);
            ring(boss.x, boss.y - 200, 300, 230, 240, 255); shake = 24; hitstop = 10; rumble_all(1.0f, 650);
        }
        if (boss.timer <= 0 && scene == SC_PLAY) show_cleared();
        break;
    }
    if (boss.state != BS_DEAD && boss.state != BS_INTRO) boss_hit_player_box(boss.x, boss.y, 150, 300);
}
static void draw_boss_vanity(float ox, float oy) {
    if (boss.pose < 0) return;
    float y = boss.y + oy + boss.bob;
    Tex *t = &A.vanity[boss.pose];
    double ang = boss.state == BS_DEAD ? sinf(frame_no * 0.5f) * 6 : sinf(boss.t * 1.3f) * 2;
    if (boss.state == VS_SPLIT) {   /* the reflections face the wrong way: that's the tell */
        for (int i = 0; i < 3; i++) { if (i != vreal && !vfake_alive[i]) continue;
            float x = sx(vfx[i]) + ox; int fake = i != vreal;
            draw_sprite(t, x, y + 10, 1, fake ? boss.face < 0 : boss.face > 0, boss.flash && !fake ? 170 : 0, fake ? 225 : 255, ang, 1.0f);
            draw_glow(&A.disc, x, y - 260, 70 + sinf(frame_no * 0.2f + i) * 10, 200, 220, 255, fake ? 50 : 70); }
        return;
    }
    draw_sprite(t, sx(boss.x) + ox, y + 10, 1, boss.face > 0, boss.flash ? 170 : 0, 255, ang, 1.0f);
    if (boss.state == VS_LIPSTICK) draw_glow(&A.disc, sx(boss.x + boss.face * 100) + ox, y - 250, 40 + sinf(frame_no * 0.6f) * 8, 255, 80, 90, 180);
}

/* ------------------------------------------------------------ boss: The Headliner (Show 7) */
enum { HS_LETTERS = 140, HS_CHASE, HS_STRIKE };
static void update_boss_headliner(void) {
    float angry = boss.hp < boss.maxhp / 2 ? 0.7f : 1.0f;
    boss.t += DT; if (boss.flash > 0) boss.flash--;
    boss.timer -= DT; boss.bob = sinf(boss.t * 1.8f) * 6;
    Player *t = nearest_player(boss.x, boss.y, NULL);
    float lo = ARENA_X + 260, hi = ARENA_X + SCREEN_W - 230;
    if (t && boss.state != BS_DEAD) boss.face = t->x < boss.x ? -1 : 1;
    if (boss.state != BS_DEAD && frame_no % 5 == 0) part(boss.x + (frand() - 0.5f) * 240, boss.y - 200 - frand() * 220, (frand() - 0.5f) * 200, -100 - frand() * 150, 0.3f, 3, 900, 255, 240, 160, 1);
    switch (boss.state) {
    case BS_INTRO: boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f; } break;
    case BS_IDLE:
        boss.pose = angry < 1 ? 2 : 0;
        if (t) { float want = t->x - boss.face * 440; boss.x += clampf(want - boss.x, -1, 1) * 90 / angry * DT; }
        boss.x = clampf(boss.x, lo, hi);
        if (boss.timer <= 0) {
            int k = rand() % 3;
            if (k == 0) { boss.state = HS_LETTERS; boss.slams = angry < 1 ? 6 : 4; boss.timer = 0.35f; message("HE'S SPELLING TROUBLE!", 1.0f); }
            else if (k == 1) { boss.state = HS_CHASE; boss.slams = angry < 1 ? 7 : 5; boss.timer = 0.5f; sfx("charge"); message("CHASE LIGHTS! JUMP!", 1.0f); }
            else { boss.state = HS_STRIKE; boss.slams = angry < 1 ? 5 : 3; boss.timer = 0.4f; sfx("roar1"); message("HE'S CALLING DOWN THE LIGHTNING!", 1.2f); }
            if (angry < 1 && !boss.summoned) { boss.summoned = 1; spawn(E_BULB, (int)((ARENA_X + 140) / TILE), 12); spawn(E_BULB, (int)((ARENA_X + 460) / TILE), 12); sfx("summon"); }
        }
        break;
    case HS_LETTERS: {  /* rips letters off the sign and throws them; they bounce */
        boss.pose = 1;
        static const char WORD[] = "MIDNIGHTMATINEE";
        if (boss.timer <= 0 && t) { float x0 = boss.x + boss.face * 80, y0 = boss.y - 320, dx = t->x - x0;
            Proj *q = proj(P_LETTER, x0, y0, clampf(dx / 1.1f, -540, 540) + (frand() - 0.5f) * 140, -700, 1, 1);
            if (q) { q->grav = 1300; q->r = 22; q->bounces = 3; q->life = 5; q->last_hit = WORD[rand() % 15]; }
            sfx("throw"); boss.timer = 0.42f * angry; if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.1f * angry; } }
        break; }
    case HS_CHASE:      /* a string of hot bulbs rolls along the roof at you */
        boss.pose = 1;
        if (boss.timer <= 0) { Proj *q = proj(P_BULB, boss.x + boss.face * 60, FLOOR_Y - 16, boss.face * 430, 0, 1, 1); if (q) { q->r = 14; q->life = 4; }
            sfx("tick"); boss.timer = 0.2f; if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.3f * angry; } }
        break;
    case HS_STRIKE:     /* lightning lands where you're standing, a moment later */
        boss.pose = angry < 1 ? 2 : 1;
        if (boss.timer <= 0 && t) { bolt_add(t->x + t->vx * 0.35f, 0.85f); boss.timer = 0.75f * angry;
            if (--boss.slams <= 0) { boss.state = BS_IDLE; boss.timer = 1.6f * angry; } }
        break;
    case BS_DEAD:
        if (boss.pose >= 0) boss.pose = 2;
        if (frame_no % 4 == 0) { sfx("boom"); spark_burst(boss.x + (frand() - 0.5f) * 220, boss.y - frand() * 340, 14, 255, 230, 140); smoke(boss.x, boss.y - 250, 2, 70);
            for (int k = 0; k < 3; k++) part_k(PK_DEBRIS, boss.x, boss.y - 300, (frand() - 0.5f) * 700, -300 - frand() * 400, 1.6f, 5 + frand() * 6, 1600, 250, 230, 160, 0); }
        if (!boss.summoned && boss.timer < 1.0f) {
            boss.summoned = 1; shatter(&A.headliner[2], boss.x, boss.y + 10, boss.face > 0, 4, 5, 900, 0); boss.pose = -1;
            for (int i = 0; i < 40; i++) part(boss.x, boss.y - 260, (frand() - 0.5f) * 1200, -300 - frand() * 800, 1.4f, 5, 900, 255, 230, 140, 1);
            bolt_add(boss.x, 0.05f); flash_t = 0.4f;
            ring(boss.x, boss.y - 200, 320, 255, 240, 180); shake = 26; hitstop = 10;
        }
        if (boss.timer <= 0 && scene == SC_PLAY) show_cleared();
        break;
    }
    if (boss.state != BS_DEAD && boss.state != BS_INTRO) boss_hit_player_box(boss.x, boss.y, 170, 320);
}
static void draw_boss_headliner(float ox, float oy) {
    if (boss.pose < 0) return;
    float x = sx(boss.x) + ox, y = boss.y + oy + boss.bob;
    Tex *t = &A.headliner[boss.pose];
    double ang = boss.state == BS_DEAD ? sinf(frame_no * 0.5f) * 6 : sinf(boss.t * 1.1f) * 1.5f;
    draw_glow(&A.disc, x, y - 260, 180 + sinf(frame_no * 0.15f) * 20, 255, 220, 120, boss.pose == 2 ? 50 : 90);
    draw_sprite(t, x, y + 10, 1, boss.face > 0, boss.flash ? 170 : 0, 255, ang, 1.0f);
    if (boss.state == HS_STRIKE || boss.state == HS_LETTERS) draw_glow(&A.disc, x + boss.face * 90, y - 220, 50 + sinf(frame_no * 0.8f) * 12, 200, 220, 255, 200);
}

/* ------------------------------------------------------------ final boss: Inky Unbound (Show 8: Encore) */
static void update_boss_unbound(void) {
    float angry = boss.hp < boss.maxhp / 2 ? 0.7f : 1.0f;
    boss.t += DT; if (boss.flash > 0) boss.flash--;
    boss.timer -= DT; boss.bob = sinf(boss.t * 1.6f) * 12;
    Player *t = nearest_player(boss.x, boss.y, NULL);
    if (t && boss.state != BS_DEAD) boss.face = t->x < boss.x ? -1 : 1;
    if (boss.state != BS_DEAD && frame_no % 3 == 0) part_k(PK_BLOOD, boss.x + (frand() - 0.5f) * 260, boss.y - 100 - frand() * 300, (frand() - 0.5f) * 60, 60, 1.2f, 4 + frand() * 5, 900, 12, 8, 12, 0);
    switch (boss.state) {
    case BS_INTRO: boss.pose = 0; if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f; } break;
    case BS_IDLE:
        boss.pose = angry < 1 ? 2 : 0;
        if (boss.timer <= 0) {
            int k = rand() % 3;
            if (k == 0) { boss.state = US_SLAM_AIM; boss.slams = angry < 1 ? 4 : 3; boss.timer = 0.8f * angry; boss.targetx = t ? t->x : boss.x; sfx("charge"); }
            else if (k == 1) { boss.state = US_RAIN; boss.timer = 2.6f; boss.slams = 0; sfx("thunder"); message("INK RAIN!", 1.0f); }
            else { boss.state = US_BEAM_WARN; boss.timer = 0.9f * angry; sfx("charge"); }
            if (angry < 1 && !boss.summoned) { boss.summoned = 1; spawn(E_SHADOW, (int)((ARENA_X + 120) / TILE), 12); spawn(E_SHADOW, (int)((ARENA_X + 420) / TILE), 12); sfx("summon"); message("HIS UNDERSTUDIES TAKE THE STAGE!", 1.5f); }
        }
        break;
    case US_SLAM_AIM:   /* an ink tentacle the size of a building hovers over you, then slams */
        boss.pose = 1;
        if (boss.timer > 0.28f && t) boss.targetx += (t->x - boss.targetx) * 0.1f;
        if (boss.timer <= 0) { boss.state = US_SLAM; boss.timer = 0.35f; shake = 18; hitstop = 3; sfx("slam"); sfx("boom");
            ring(boss.targetx, FLOOR_Y - 10, 180, 60, 40, 60); splat(boss.targetx, FLOOR_Y + 2, 70, 1, 10, 8, 10);
            for (int k = 0; k < 24; k++) part_k(PK_BLOOD, boss.targetx + (frand() - 0.5f) * 120, FLOOR_Y - 6, (frand() - 0.5f) * 700, -200 - frand() * 500, 1.3f, 5 + frand() * 6, 1500, 12, 8, 12, 0); }
        break;
    case US_SLAM:
        boss.pose = 1; boss_hit_player_box(boss.targetx, FLOOR_Y, 140, FLOOR_Y);
        if (boss.timer <= 0) { if (--boss.slams > 0) { boss.state = US_SLAM_AIM; boss.timer = 0.55f * angry; } else { boss.state = BS_IDLE; boss.timer = 1.2f * angry; } }
        break;
    case US_RAIN:       /* ink pours from the rafters around you */
        boss.pose = 0;
        if (frame_no % (angry < 1 ? 5 : 8) == 0) { float x = (t ? t->x : boss.x) + (frand() - 0.5f) * 700;
            Proj *q = proj(P_DRIP, clampf(x, ARENA_X + 40, ARENA_X + SCREEN_W - 40), 10, 0, 80, 1, 1); if (q) { q->grav = 1500; q->r = 12; } }
        if (boss.timer <= 0) { boss.state = BS_IDLE; boss.timer = 1.0f * angry; }
        break;
    case US_BEAM_WARN: boss.pose = 1; if (boss.timer <= 0) { boss.state = US_BEAM; boss.sweepx = boss.x + boss.face * 80; sfx("beam"); } break;
    case US_BEAM:       /* his eyes burn a line across the stage floor: jump it */
        boss.pose = 1; boss.sweepx += boss.face * 760 / angry * DT;
        boss_hit_player_box(boss.sweepx, FLOOR_Y, 70, 60);
        if (frame_no % 2 == 0) { part(boss.sweepx, FLOOR_Y - 8, (frand() - 0.5f) * 300, -200 - frand() * 200, 0.4f, 6, 600, 255, 180, 60, 1); if (frame_no % 6 == 0) splat(boss.sweepx, FLOOR_Y + 2, 14, 1, 30, 14, 6); }
        if (boss.sweepx < ARENA_X - 60 || boss.sweepx > ARENA_X + SCREEN_W + 60) { boss.state = BS_IDLE; boss.timer = 1.0f * angry; }
        break;
    case BS_DEAD:
        if (boss.pose >= 0) boss.pose = 2;
        if (frame_no % 3 == 0) { sfx("boom"); ink_burst(boss.x + (frand() - 0.5f) * 300, boss.y - frand() * 420, 14, 380);
            spark_burst(boss.x + (frand() - 0.5f) * 260, boss.y - frand() * 400, 10, 255, 170, 60);
            if (frame_no % 9 == 0) gore_part(rand() % 12, boss.x + (frand() - 0.5f) * 240, boss.y - 160 - frand() * 260, 800, 1); }
        if (!boss.summoned && boss.timer < 1.2f) {
            boss.summoned = 1; shatter(&A.unbound[2], boss.x, boss.y + 10, boss.face > 0, 6, 6, 950, 1); boss.pose = -1;
            for (int i = 0; i < 80; i++) { float a = frand() * 6.283f, v = 300 + frand() * 800; part_k(PK_BLOOD, boss.x, boss.y - 260, cosf(a) * v, sinf(a) * v - 300, 1.8f, 6 + frand() * 10, 1500, 12, 8, 12, 0); }
            for (int k = 0; k < 4; k++) ring(boss.x, boss.y - 260, 300 + k * 120, 255, 200, 120);
            blood_spurt(boss.x, boss.y - 300, -1, 60, 900); blood_spurt(boss.x, boss.y - 300, 1, 60, 900);
            shake = 30; hitstop = 14; negative_t = 0.4f;
        }
        if (boss.timer <= 0 && scene == SC_PLAY) show_cleared();
        break;
    }
    if (boss.state != BS_DEAD && boss.state != BS_INTRO) boss_hit_player_box(boss.x, boss.y, 260, 300);
}
static void draw_boss_unbound(float ox, float oy) {
    float x = sx(boss.x) + ox, y = boss.y + oy + boss.bob;
    if (boss.state == US_SLAM_AIM || boss.state == US_SLAM) {   /* a colossal ink tentacle */
        float tx = sx(boss.targetx) + ox, k = boss.state == US_SLAM ? 1 : clampf(1 - boss.timer / 0.8f, 0, 1);
        float tip = boss.state == US_SLAM ? FLOOR_Y + oy : 150 + oy + sinf(frame_no * 0.7f) * 8;
        if (boss.state == US_SLAM_AIM) { draw_ellipse(&A.disc, tx, FLOOR_Y + oy, 60 + 30 * k, 10 + 5 * k, 0, 0, 0, (Uint8)(110 + 130 * k));
            if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", tx, FLOOR_Y + oy - 80, 0.8f, 1, 255, 60, 40, 255); }
        for (int i = 0; i < 14; i++) { float yy = tip - i * 48, ww = 34 + i * 3, wob = sinf(frame_no * 0.2f + i * 0.6f) * 8;
            draw_ellipse(&A.disc, tx + wob, yy, ww, 34, 10, 8, 12, 255); }
        for (int i = 1; i < 12; i += 2) draw_ellipse(&A.disc, tx + sinf(frame_no * 0.2f + i * 0.6f) * 8 - boss.face * 18, tip - i * 48, 9, 7, 70, 60, 80, 255);   /* suckers */
        draw_ellipse(&A.disc, tx, tip - 10, 26, 18, 230, 225, 210, 255);   /* one of his old white gloves */
    }
    if (boss.state == US_BEAM) { float bx = sx(boss.sweepx) + ox, ex = x + boss.face * 40, ey = y - 330;
        draw_bar(ex, ey, bx, FLOOR_Y + oy - 6, 22, 255, 140, 30, 150); draw_bar(ex, ey, bx, FLOOR_Y + oy - 6, 8, 255, 230, 150, 255);
        draw_glow(&A.disc, bx, FLOOR_Y + oy - 10, 50, 255, 160, 60, 255); }
    if (boss.state == US_BEAM_WARN) draw_glow(&A.disc, x + boss.face * 40, y - 330, 40 + (0.9f - boss.timer) * 50, 255, 150, 40, 255);
    if (boss.pose < 0) return;
    Tex *t = &A.unbound[boss.pose];
    double ang = boss.state == BS_DEAD ? sinf(frame_no * 0.6f) * 6 : sinf(boss.t * 1.1f) * 2;
    draw_sprite(t, x, y + 30, 1, boss.face > 0, boss.flash ? 170 : 0, 255, ang, 1.0f + 0.02f * sinf(boss.t * 3));
}
/* ------------------------------------------------------------ projectiles, pickups, particles */
static void update_proj(void) {
    for (int i = 0; i < MAXP; i++) {
        Proj *p = &pr[i]; if (!p->alive) continue;
        p->life -= DT; if (p->life <= 0) { p->alive = 0; continue; }
        p->vy += p->grav * DT; p->x += p->vx * DT; p->y += p->vy * DT;
        if (p->x < camx - 100 || p->x > camx + SCREEN_W + 100 || p->y > LH * TILE + 50) { p->alive = 0; continue; }
        int tx = (int)(p->x / TILE), ty = (int)(p->y / TILE);
        if (solid_at(tx, ty) && p->y > 0) {
            if (map[ty][tx] == 'Y' && mirror_hit(p, tx)) continue;
            if (p->type == P_LETTER && p->vy >= 0 && p->bounces > 0) { p->bounces--; p->vy = -560; p->y = ty * TILE - 1; sfx("clank"); spark_burst(p->x, p->y, 6, 255, 120, 200); continue; }
            if (p->type == P_SHARD) { sfx("shatter"); for (int k = 0; k < 6; k++) part_k(PK_DEBRIS, p->x, p->y, (frand() - 0.5f) * 400, -150 - frand() * 250, 1.2f, 3 + frand() * 4, 1600, 200, 225, 240, 0); }
            if (p->type == P_SPARK || p->type == P_BULB) { spark_burst(p->x, p->y, 10, 255, 240, 150); }
            if (p->type == P_NOTE && p->bounces > 0) { p->bounces--; p->vy = -650; p->y = ty * TILE - 1; sfx("boing"); splat(p->x, ty * TILE, 14, 1, 12, 8, 12); continue; }
            if (p->type == P_REEL && p->vy >= 0) { p->y = ty * TILE - 1;
                if (p->vy > 260 && p->bounces > 0) { p->bounces--; p->vy = -p->vy * 0.45f; sfx("clank"); spark_burst(p->x, p->y, 6, 255, 255, 240); }
                else { p->vy = 0; p->grav = 0; }
                continue; }
            if (p->type == P_PIZZA && p->vy >= 0 && p->bounces > 0) { p->bounces--; p->vy = -380; p->y = ty * TILE - 1; splat(p->x, ty * TILE, 10, 1, 170, 30, 20); continue; }
            if (p->type == P_CONFETTI) { for (int i = 0; i < 12; i++) part(p->x, p->y, (frand() - 0.5f) * 400, -100 - frand() * 300, 0.8f, 4, 900, 120 + rand() % 136, 80 + rand() % 176, 80 + rand() % 176, 0); spark_burst(p->x, p->y, 6, 255, 220, 120); }
            if (p->type == P_SHOE && p->vy >= 0 && p->bounces > 0) { p->bounces--; p->vy = -360; p->y = ty * TILE - 1; sfx("boing"); continue; }
            if (p->type == P_CORK) { spark_burst(p->x, p->y, 5, 230, 200, 150); }
            if (p->type == P_INKBLOB) { sfx("splat"); for (int i = 0; i < 10; i++) part_k(PK_BLOOD, p->x, p->y - 4, (frand() - 0.5f) * 400, -100 - frand() * 250, 1.0f, 4 + frand() * 4, 1400, 12, 8, 12, 0); splat(p->x, ty * TILE + 2, 22, 1, 10, 8, 10); }
            if (p->type == P_DRIP || p->type == P_NOTE) { ink_burst(p->x, p->y, 6, 140); splat(p->x, ty * TILE, 12, 1, 12, 8, 12); }
            else if (p->type == P_RIVET) { spark_burst(p->x, p->y, 10, 255, 160, 60); smoke(p->x, p->y, 2, 90); ring(p->x, p->y, 40, 255, 170, 80); }
            else if (p->type == P_NAIL) { spark_burst(p->x, p->y, 5, 255, 230, 160); splat(p->x, p->y, 3, 0, 40, 36, 34); }
            else if (p->type == P_PLUNGER) { sfx("boing"); splat(p->x, p->y, 9, 0, 170, 30, 30); }
            else if (p->type == P_FLAME) { embers(p->x, p->y, 2); if (rand() % 3 == 0) splat(p->x, p->y, 6, 0, 30, 20, 14); }
            else if (!p->enemy) spark_burst(p->x, p->y, 4, 255, 220, 150);
            if (p->type == P_LIGHT) { spark_burst(p->x, p->y, 8, 255, 240, 170); splat(p->x, p->y, 8, 0, 40, 28, 16); }
            p->alive = 0; continue;
        }
        /* trails */
        if (p->type == P_FLAME) { if (frame_no % 2 == 0) part(p->x, p->y, p->vx * 0.1f, -60, 0.2f, 10, -100, 140, 190, 255, 1); if (rand() % 4 == 0) embers(p->x, p->y, 1); }
        if (p->type == P_RIVET && frame_no % 2 == 0) part(p->x, p->y, 0, 0, 0.18f, 6, 0, 255, 120, 30, 1);
        if (p->type == P_LIGHT && frame_no % 2 == 0) part(p->x, p->y, 0, 0, 0.25f, 9, 0, 255, 230, 150, 1);
        if (p->type == P_NOTE && frame_no % 3 == 0) part_k(PK_BLOOD, p->x, p->y, 0, 0, 1.0f, 4, 900, 12, 8, 12, 0);
        if (p->type == P_PLUNGER) p->r = 14;
        if (p->type == P_INKBLOB && frame_no % 3 == 0) part_k(PK_BLOOD, p->x, p->y, 0, 0, 0.8f, 3, 900, 12, 8, 12, 0);
        if (p->type == P_SHEET) { p->vy = sinf(p->life * 7 + p->x * 0.01f) * 220; }
        if (p->type == P_FILM && frame_no % 3 == 0) part(p->x, p->y, 0, 0, 0.3f, 6, 0, 200, 196, 186, 0);
        if (p->type == P_LIPSTICK) {   /* steers after the nearest runner */
            Player *t = nearest_player(p->x, p->y, NULL);
            if (t && p->life > 1.2f) { float dx = t->x - p->x, dy = (t->y - 70) - p->y, l = sqrtf(dx * dx + dy * dy) + 1, sp = sqrtf(p->vx * p->vx + p->vy * p->vy);
                p->vx += (dx / l * sp - p->vx) * 1.6f * DT; p->vy += (dy / l * sp - p->vy) * 1.6f * DT; }
            if (frame_no % 2 == 0) part(p->x, p->y, 0, 0, 0.3f, 7, 0, 255, 90, 100, 1); }
        if (p->type == P_SHARD && p->bounces > 0) {   /* hangs from the ceiling, trembling, then drops */
            p->vy = 0; p->vx = 0; p->y = 40; p->x += sinf(frame_no * 1.7f) * 1.5f;
            if (p->life < 2.55f) { p->bounces = 0; p->grav = 2300; sfx("shatter"); } }
        if (p->type == P_KISS) { p->vy += sinf(p->life * 6) * 260 * DT; if (frame_no % 4 == 0) part(p->x, p->y, 0, 0, 0.3f, 6, -60, 255, 120, 170, 1); }
        if ((p->type == P_SPARK || p->type == P_BULB) && frame_no % 2 == 0) part(p->x - signf(p->vx) * 10, p->y, (frand() - 0.5f) * 80, -60 - frand() * 120, 0.25f, 3, 600, 255, 240, 140, 1);
        if (p->enemy) {
            for (int k = 0; k < 2; k++) { Player *q = &pl[k];
                if (q->active && q->alive && !q->hidden && overlap(q->x, q->y, 40, q->crouch ? 76 : 118, p->x, p->y + p->r, p->r * 2, p->r * 2)) {
                    hurt_player(q, p->x); if (p->type != P_NOTE) p->alive = 0; } }
        } else {
            for (int j = 0; j < nen && p->alive; j++) { Enemy *e = &en[j]; if (!e->alive || j == p->last_hit || ghost_faded(e) || (e->type == E_EEL && (e->state == 0 || e->state == 3))) continue;
                float w, h; enemy_box(e, &w, &h);
                if (overlap(e->x, e->y, w, h, p->x, p->y + p->r, p->r * 2, p->r * 2)) {
                    if (p->type == P_FLAME) e->burn = 2.2f;                      /* set them on fire */
                    if (p->type == P_RIVET) { ring(p->x, p->y, 50, 255, 170, 80); smoke(p->x, p->y, 2, 90); }
                    if (p->type == P_PLUNGER) { e->stun = 0.6f; sfx("boing"); message("THWONK!", 0.5f); }
                    enemy_damage(e, p->dmg); spark_burst(p->x, p->y, 7, 255, 240, 200);
                    if (p->type == P_FLAME) p->last_hit = j; else p->alive = 0; } }
            if (p->alive && vanity_fake_hit(p)) continue;
            if (p->alive && boss.active && boss.state != BS_DEAD && boss.state != BS_INTRO && boss.state != SS_FLICK_OUT && !(level_no == L_LOBBY && boss.state == BS_WARP) &&
                overlap(boss.x, boss.y + boss.bob - 40, 220, 360, p->x, p->y + p->r, p->r * 2, p->r * 2)) {
                boss.hp -= p->dmg; boss.flash = 5; spark_burst(p->x, p->y, 6, 255, 240, 200);
                if (rand() % 3 == 0) blood_spurt(p->x, p->y, p->vx > 0 ? -1 : 1, 4, 400);
                if (p->type != P_FLAME) p->alive = 0; else p->life -= 0.1f;
                if (frame_no % 6 == 0) sfx("hit");
                if (boss.hp <= 0) {   /* his band dies with him */
                    for (int j = 0; j < nen; j++) if (en[j].alive && en[j].type < E_CRATE) { gore_enemy(&en[j]); en[j].alive = 0; }
                    for (int j = 0; j < MAXP; j++) if (pr[j].enemy) pr[j].alive = 0;
                    boss.pose = 2;
                }
                if (boss.hp <= 0) { boss.summoned = 0; boss.state = BS_DEAD; boss.timer = 3.5f; score += 10000; message(LEVELS[level_no].kill_msg, 3.0f); shake = 16; sfx("roar2"); hitstop = 18; }
                else if (boss.hp < boss.maxhp / 2 && boss.hp + p->dmg >= boss.maxhp / 2) { sfx("angry"); message(LEVELS[level_no].angry_msg, 1.5f); shake = 10; }
            }
        }
    }
    for (int i = 0; i < 8; i++) { Pickup *k = &pick[i]; if (!k->alive) continue;
        k->t += DT; k->vy += 1400 * DT; float vx = 0;
        move_box(&k->x, &k->y, &vx, &k->vy, 40, 40, 0); if (k->t > 14) k->alive = 0; }
    for (int i = 0; i < MAXPART; i++) { Part *q = &parts[i]; if (q->life <= 0) continue;
        q->life -= DT; q->vy += q->grav * DT; q->x += q->vx * DT; q->y += q->vy * DT; q->ang += q->vang * DT;
        if (q->kind == PK_SMOKE) { q->size += 18 * DT; q->vx *= 0.98f; }
        if (q->kind == PK_BLOOD || q->kind == PK_CASING || q->kind == PK_DEBRIS) {
            int tx = (int)(q->x / TILE), ty = (int)(q->y / TILE);
            int hit = solid_at(tx, ty) || map[ty < 0 ? 0 : ty >= LH ? LH - 1 : ty][tx < 0 ? 0 : tx >= LW ? LW - 1 : tx] == '~';
            if (hit && q->y > 0) {
                if (q->kind == PK_BLOOD) {   /* lands: leaves a stain on the floor or wall */
                    int floor = q->vy > 0 && !solid_at(tx, (int)((q->y - 14) / TILE));
                    splat(q->x, floor ? ty * TILE + 2 : q->y, q->size * 1.2f + 2, floor, q->r, q->g, q->b); q->life = 0;
                } else {                     /* casings and splinters bounce and settle */
                    if (q->kind == PK_CASING && q->vy > 300 && rand() % 4 == 0 && q->x > camx && q->x < camx + SCREEN_W) sfx("casing");
                    q->y = ty * TILE - 1; q->vy *= -0.35f; q->vx *= 0.6f; q->vang *= 0.5f;
                    if (fabsf(q->vy) < 60) { q->vy = 0; q->grav = 0; q->vx = 0; q->vang = 0; }
                }
            }
        }
    }
    /* gibs */
    for (int i = 0; i < MAXGIB; i++) { Gib *g = &gibs[i]; if (g->life <= 0) continue;
        g->life -= DT; if (g->rest) continue;
        g->vy += 1700 * DT; g->x += g->vx * DT; g->y += g->vy * DT; g->ang += g->vang * DT;
        if (g->bleed && frame_no % 4 == 0) part_k(PK_BLOOD, g->x, g->y, (frand() - 0.5f) * 40, 0, 1.0f, 3 + frand() * 2, 1200, 140, 12, 16, 0);
        int tx = (int)(g->x / TILE), ty = (int)((g->y + g->h * 0.3f) / TILE);
        if (solid_at(tx, ty) && g->vy > 0) {
            g->y = ty * TILE - g->h * 0.3f; g->vy *= -0.4f; g->vx *= 0.55f; g->vang *= 0.5f;
            if (g->bleed) splat(g->x, ty * TILE + 2, 10, 1, 130, 10, 14);
            if (fabsf(g->vy) < 80) { g->rest = 1; g->vang = 0; }
        }
        if (g->y > LH * TILE + 100) g->life = 0;
    }
    for (int i = 0; i < 16; i++) if (rings[i].life > 0) { rings[i].life -= DT; rings[i].r += (rings[i].max - rings[i].r) * 0.25f; }
    /* burning enemies */
    for (int i = 0; i < nen; i++) { Enemy *e = &en[i]; if (!e->alive || e->burn <= 0) continue;
        float w, h; enemy_box(e, &w, &h);
        e->burn -= DT; e->btick -= DT;
        if (frame_no % 2 == 0) part(e->x + (frand() - 0.5f) * w, e->y - frand() * h, (frand() - 0.5f) * 40, -120, 0.35f, 9 + frand() * 6, -150, 255, 120 + rand() % 90, 30, 1);
        if (frame_no % 6 == 0) { smoke(e->x, e->y - h, 1, 50); embers(e->x, e->y - h * 0.5f, 1); }
        if (e->btick <= 0) { e->btick = 0.45f; enemy_damage(e, 1); }
    }
}

/* ------------------------------------------------------------ security office (GamePad) actions */
static void op_ping(void) {
    if (ping_cool > 0 || power < 6) { sfx("tick"); return; }
    power -= 6; ping_cool = 1.0f; int n = 0;
    for (int i = 0; i < nen; i++) if (en[i].alive && zone_of(en[i].x) == cam_sel && en[i].type != E_CRATE && en[i].type != E_HATCRATE) { en[i].marked = 6; n++; }
    sfx("coin"); cam_flash = 0.3f;
    char b[48]; snprintf(b, sizeof b, "OFFICE PINGED %d TARGET%s", n, n == 1 ? "" : "S"); message(b, 1.4f);
}
static void op_lights(void) {
    if (power < 10 || lit[cam_sel] > 0) { sfx("tick"); return; }
    power -= 10; lit[cam_sel] = 8.0f; sfx("lights"); cam_flash = 0.4f;
    char b[48]; snprintf(b, sizeof b, "LIGHTS UP: %s", ZONE_NAME[cam_sel]); message(b, 1.4f);
}
static void auto_camera(void) {   /* nobody on the cameras for a while: follow the action */
    op_idle += DT; if (op_idle < 6.0f) return;
    int z = zone_of(pl[0].active ? pl[0].x : pl[1].x);
    if (fiend.state >= 1) z = 3;
    if (boss.active) z = 5;
    if (z != cam_sel && frame_no % 30 == 0) { cam_sel = z; cam_flash = 0.15f; }
}
static void op_input(const Input *in) {
    if (in->pressed & (B_CAMPREV | B_CAMNEXT | B_PING | B_LIGHTS)) op_idle = 0;
    if (in->pressed & B_CAMPREV) { cam_sel = (cam_sel + 5) % 6; sfx("menu"); cam_flash = 0.15f; }
    if (in->pressed & B_CAMNEXT) { cam_sel = (cam_sel + 1) % 6; sfx("menu"); cam_flash = 0.15f; }
    if (in->pressed & B_PING) op_ping();
    if (in->pressed & B_LIGHTS) op_lights();
}

/* GamePad button layout (854x480) shared by drawing and touch */
static SDL_Rect cam_btn(int i) { return (SDL_Rect){560 + (i % 2) * 142, 58 + (i / 2) * 82, 134, 74}; }
static const SDL_Rect BTN_PING = {16, 404, 170, 62}, BTN_LIGHTS = {196, 404, 170, 62};
static int in_rect(SDL_Rect r, float x, float y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

void game_touch(float fx, float fy) {
    float x = fx * PAD_W, y = fy * PAD_H;
    if (scene != SC_PLAY) return;
    op_idle = 0;
    for (int i = 0; i < 6; i++) if (in_rect(cam_btn(i), x, y)) { cam_sel = i; sfx("menu"); cam_flash = 0.15f; return; }
    if (in_rect(BTN_PING, x, y)) op_ping();
    if (in_rect(BTN_LIGHTS, x, y)) op_lights();
}

/* ------------------------------------------------------------ step */
static void game_step_inner(const Input pads[MAX_PADS], const int connected[MAX_PADS]) {
    frame_no++; scene_t += DT;
    if (shake > 0) shake = fmaxf(0, shake - 30 * DT);
    if (msg_t > 0) msg_t -= DT;
    if (warn_t > 0) warn_t -= DT;
    if (cam_flash > 0) cam_flash -= DT;
    if (ping_cool > 0) ping_cool -= DT;
    unsigned anyp = 0; for (int i = 0; i < MAX_PADS; i++) anyp |= pads[i].pressed;
    static Input fake[MAX_PADS];
    if (demo_on) {   /* attract mode: any button hands the game back */
        if (anyp) { end_demo(); return; }
        demo_t += DT;
        if (demo_t > 40 || scene == SC_OVER || scene == SC_CLEAR || scene == SC_END) { end_demo(); return; }
        memcpy(fake, pads, sizeof fake); fake[0] = demo_input(); pads = fake; anyp = fake[0].pressed & ~B_START;
    }

    for (int i = 0; i < MAX_PADS; i++) con_now[i] = connected[i];
    unsigned anyh = 0; for (int i = 0; i < MAX_PADS; i++) anyh |= pads[i].held;
    if (scene != SC_PLAY) { step_front(pads, anyp, anyh); return; }
    /* pause: + on a runner's controller (or the camera operator's GamePad) */
    if (paused) { step_pause(anyp); return; }
    { unsigned st = 0; for (int i = 0; i < 2; i++) if (pl[i].active) st |= pads[pl[i].pad].pressed; if (mode == 1) st |= pads[op_pad].pressed;
      if (st & B_START) { paused = 1; pause_sel = 0; audio_duck(1); sfx("menu"); return; } }
    show_time += DT; if (rush_on) rush_time += DT;
    if (shake > last_shake + 6) rumble_all(fminf(1, shake / 30), 140);   /* big hits buzz the controllers */
    last_shake = shake;
    if (hitstop > 0) { hitstop--; return; }
    /* drop-in second runner */
    int p2pad = mode == 1 ? 2 : 1;
    if (!pl[1].active && connected[p2pad] && (pads[p2pad].pressed & B_START) && pl[0].active) {
        reset_player(&pl[1], 1); pl[1].active = 1; pl[1].pad = p2pad; pl[1].x = pl[0].x - 50; pl[1].y = pl[0].y; message("PLAYER 2 CLOCKS IN!", 1.5f); sfx("checkpoint");
    }
    /* power drains; at zero the lights go out */
    float drain = (0.45f + (boss.active ? 0.25f : 0)) * (difficulty() == 0 ? 0.5f : 1.0f);
    power = clampf(power - drain * DT, 0, 100);
    for (int z = 0; z < 6; z++) if (lit[z] > 0) lit[z] -= DT;
    int cz = zone_of(camx + SCREEN_W / 2);
    int was_dark = darkness_on;
    darkness_on = power <= 0 && lit[cz] <= 0;
    if (darkness_on && !was_dark && power <= 0) { sfx("thunder"); message("POWER OUT! FIND A BREAKER", 2.0f); }

    if (!boss.active) {   /* each area has its own take on the theme; missing tracks fall back to the main one */
        const char *const *ZONE_MUSIC = LEVELS[level_no].music;
        Player *lead = pl[0].active ? &pl[0] : &pl[1];
        const char *want = ZONE_MUSIC[zone_of(lead->x)];
        static int avail[NUM_LEVELS][6], avail_init = 0;
        if (!avail_init) { memset(avail, -1, sizeof avail); avail_init = 1; }
        int z = zone_of(lead->x);
        if (avail[level_no][z] < 0) avail[level_no][z] = music_available(want);
        music_play(avail[level_no][z] ? want : LEVELS[level_no].music[0], 1);
    }
    for (int i = 0; i < 2; i++) update_player(&pl[i], &pads[pl[i].pad]);
    op_input(&pads[op_pad]); auto_camera();
    if (negative_t > 0) negative_t -= DT;
    update_reels(); update_enemies(); update_fiend(); update_ovens(); update_saws(); update_crushers_vents(); update_blocks_traps(); update_tide(); update_storm(); update_boss(); update_proj();

    /* camera */
    float tx = 0; int n = 0;
    for (int i = 0; i < 2; i++) if (pl[i].active && (pl[i].alive || pl[i].dead_t < 1)) { tx += pl[i].x; n++; }
    if (n) {
        tx /= n;
        float want = tx - SCREEN_W * 0.42f;
        if (arena_locked) want = ARENA_X;
        want = clampf(want, 0, LW * TILE - SCREEN_W);
        camx += (want - camx) * (arena_locked ? 0.08f : 0.18f);
    }
    /* game over */
    if (!pl[0].active && !pl[1].active) { scene = SC_OVER; scene_t = 0; over_sel = 0; music_play("mm_gameover", 0); if (!demo_on) sfx("ann_over"); }
}

/* ------------------------------------------------------------ drawing: TV */
static float sx(float x) { return x - camx; }

static void draw_tiles(float ox, float oy) {
    int t0 = (int)(camx / TILE) - 1, t1 = t0 + SCREEN_W / TILE + 3;
    for (int ty = 0; ty < LH; ty++) for (int tx = t0; tx <= t1; tx++) {
        if (tx < 0 || tx >= LW) continue;
        char c = map[ty][tx]; float x = sx(tx * TILE) + ox, y = ty * TILE + oy;
        if (c == '#' && zground.tex) {   /* painted ground texture, mapped in world space so it tiles across blocks */
            const int span = 192; int top = ty == 0 || map[ty - 1][tx] != '#';
            int wx = (tx * TILE) % span, wy = (ty * TILE) % span;
            SDL_Rect src = {wx * zground.w / span, wy * zground.h / span, TILE * zground.w / span, TILE * zground.h / span};
            draw_tex_src(&zground, &src, x, y, TILE, TILE, 255);
            if (top && ty > 0) { fill_rect((int)x, (int)y, TILE, 4, 255, 236, 200, 70); fill_rect((int)x, (int)y + 4, TILE, 3, 0, 0, 0, 110); }
            if (map[ty][tx > 0 ? tx - 1 : 0] != '#') fill_rect((int)x, (int)y, 3, TILE, 0, 0, 0, 120);
            if (tx < LW - 1 && map[ty][tx + 1] != '#') fill_rect((int)x + TILE - 3, (int)y, 3, TILE, 0, 0, 0, 120);
        } else if (c == '#' && level_no == 3) {   /* ink works: riveted iron */
            int top = ty == 0 || map[ty - 1][tx] != '#';
            fill_rect((int)x, (int)y, TILE, TILE, 54, 46, 40, 255);
            fill_rect((int)x, (int)y, 2, TILE, 30, 26, 22, 255); fill_rect((int)x, (int)y + TILE - 2, TILE, 2, 30, 26, 22, 255);
            draw_ellipse(&A.disc, x + 8, y + 8, 3, 3, 130, 110, 80, 255); draw_ellipse(&A.disc, x + 40, y + 8, 3, 3, 130, 110, 80, 255);
            if (top && ty > 0) { fill_rect((int)x, (int)y, TILE, 6, 120, 100, 70, 255); if ((tx * 13) % 5 == 0) draw_ellipse(&A.disc, x + 24, y + 9, 6, 10 + (tx % 3) * 4, 10, 8, 10, 255); }
        } else if (c == '#' && level_no == 2) {   /* old cartoon film: grey */
            int top = ty == 0 || map[ty - 1][tx] != '#';
            fill_rect((int)x, (int)y, TILE, TILE, 70, 68, 66, 255);
            fill_rect((int)x, (int)y + 23, TILE, 2, 36, 34, 34, 255);
            fill_rect((int)x + ((tx * 7 + ty * 3) % 3) * 16, (int)y, 2, TILE, 40, 38, 38, 255);
            if (top) { fill_rect((int)x, (int)y, TILE, 6, 170, 166, 160, 255); fill_rect((int)x, (int)y + 6, TILE, 3, 16, 14, 14, 255); }
        } else if (c == 'F') {   /* a frame of film you can stand on... while it's in the picture */
            float ph = film_phase(tx); int on = film_on(tx), warn = on && ph > 1.6f && (frame_no / 3) % 2;
            if (on && !warn) {
                fill_rect((int)x, (int)y, TILE, 16, 22, 20, 20, 255);
                fill_rect((int)x + 2, (int)y + 4, TILE - 4, 8, 180, 176, 168, 255);
                for (int k = 0; k < 3; k++) { fill_rect((int)x + 4 + k * 16, (int)y + 1, 7, 3, 240, 236, 226, 255); fill_rect((int)x + 4 + k * 16, (int)y + 12, 7, 3, 240, 236, 226, 255); }
                fill_rect((int)x + TILE - 2, (int)y, 2, 16, 10, 10, 10, 255);
            } else {
                for (int k = 0; k < 4; k++) fill_rect((int)x + k * 12, (int)y + 6, 6, 3, 200, 196, 186, on ? 160 : 55);
                if (!on && ph > 2.9f && (frame_no / 3) % 2) fill_rect((int)x, (int)y + 4, TILE, 8, 220, 216, 206, 90);
            }
        } else if (c == 'Y') {   /* a tall dressing-room mirror */
            int top = ty == 0 || map[ty - 1][tx] != 'Y', bot = ty == LH - 1 || map[ty + 1][tx] != 'Y', hp = mirror_hp[tx];
            fill_rect((int)x + 4, (int)y, TILE - 8, TILE, 150, 120, 50, 255);   /* gilt frame */
            fill_rect((int)x + 9, (int)y + (top ? 6 : 0), TILE - 18, TILE - (top ? 6 : 0) - (bot ? 6 : 0), 120, 150, 170, 255);
            { float g = fmodf(scene_t * 60 + tx * 40 + ty * 18, 120) - 30; if (g > 0 && g < TILE) fill_rect((int)x + 12, (int)(y + g), TILE - 24, 6, 230, 245, 255, 120); }
            if (hp <= 3) draw_bar(x + 12, y + 8, x + TILE - 14, y + 30 + (tx % 3) * 4, 2, 240, 250, 255, 220);
            if (hp <= 2) draw_bar(x + TILE - 12, y + 10, x + 14, y + TILE - 8, 2, 240, 250, 255, 220);
            if (hp <= 1) { draw_bar(x + 24, y, x + 18, y + TILE, 2, 255, 255, 255, 240); draw_bar(x + 10, y + 24, x + TILE - 10, y + 20, 2, 255, 255, 255, 240); }
            if (top) for (int k = 0; k < 3; k++) draw_ellipse(&A.disc, x + 10 + k * 14, y + 2, 4, 4, 255, 230, 150, 255);
        } else if (c == 'n') {   /* a giant neon letter: you can stand on it while it's lit */
            static const char SIGN[] = "MIDNIGHT MATINEE ";
            float ph = neon_phase(tx); int on = neon_on(tx), warn = on && ph > 2.0f && (frame_no / 3) % 2, pink = (tx / 4) % 2;
            char b[2] = {SIGN[tx % 17], 0};
            fill_rect((int)x, (int)y + 12, TILE, 6, 50, 46, 50, 255);   /* the girder it hangs on */
            if (on && !warn) { draw_glow(&A.disc, x + 24, y + 6, 44, pink ? 255 : 80, pink ? 80 : 210, pink ? 180 : 255, 150);
                fill_rect((int)x, (int)y, TILE, 4, pink ? 255 : 150, pink ? 160 : 235, pink ? 220 : 255, 255);
                if (b[0] != ' ') draw_text_col(&A.font, b, x + 24, y - 40, 0.95f, 1, pink ? 255 : 190, pink ? 190 : 245, pink ? 230 : 255, 255); }
            else if (b[0] != ' ') draw_text_col(&A.font, b, x + 24, y - 40, 0.95f, 1, 70, 60, 70, on ? 200 : 255);
        } else if (c == 'I') {   /* lightning rod */
            draw_bar(x + 24, y + TILE, x + 24, y - 40, 5, 110, 100, 90, 255); draw_ellipse(&A.disc, x + 24, y - 42, 6, 6, 170, 160, 140, 255);
            draw_bar(x + 14, y + 10, x + 34, y + 10, 3, 110, 100, 90, 255);
        } else if (c == 'U') {   /* wobbly toy block */
            float ct = crumble_t[ty][tx], wob = ct > 0 ? sinf(frame_no * 1.7f) * 3 * (ct / 0.55f + 0.3f) : 0;
            Uint8 br = (Uint8)(150 + (tx * 37) % 100), bg_ = (Uint8)(60 + (tx * 53) % 140), bb = (Uint8)(50 + (tx * 71) % 120);
            fill_rect((int)(x + wob), (int)y, TILE, TILE - 4, 30, 20, 14, 255); fill_rect((int)(x + wob) + 3, (int)y + 3, TILE - 6, TILE - 10, br, bg_, bb, 255);
            fill_rect((int)(x + wob) + 9, (int)y + 9, TILE - 18, TILE - 22, (Uint8)(br * 0.8f), (Uint8)(bg_ * 0.8f), (Uint8)(bb * 0.8f), 255);
            { char letter[2] = {(char)('A' + (tx * 7 + ty) % 26), 0}; draw_text_col(&A.small, letter, x + wob + 24, y + 10, 0.5f, 1, 250, 240, 210, 255); }
        } else if (c == 'u') {   /* a crumbled block, stacking itself back up */
            if (regrow_t[ty][tx] < 0.8f && (frame_no / 3) % 2) fill_rect((int)x + 3, (int)y + 3, TILE - 6, TILE - 10, 220, 200, 160, 70);
        } else if (c == 'M') {   /* giant mousetrap */
            float tt = trap_t[tx]; int snapped = tt >= 0.28f && tt < 2.0f;
            fill_rect((int)x - 10, (int)y + 34, TILE + 20, 14, 40, 26, 14, 255); fill_rect((int)x - 8, (int)y + 36, TILE + 16, 10, 170, 120, 70, 255);
            draw_ellipse(&A.disc, x + 24, y + 32, 9, 7, 255, 210, 80, 255);   /* cheese */
            if (snapped) { draw_bar(x - 6, y + 34, x + 54, y + 34, 5, 200, 200, 210, 255); draw_bar(x - 6, y + 34, x - 6, y + 24, 5, 200, 200, 210, 255); }
            else { float a = tt > 0 ? 1.2f + tt * 3 : 1.5708f;   /* the bar, cocked back */
                draw_bar(x - 6, y + 40, x - 6 + cosf(a) * 60, y + 40 - sinf(a) * 60, 5, 200, 200, 210, 255);
                if (tt > 0 && (frame_no / 2) % 2) draw_text_col(&A.small, "!", x + 24, y - 10, 0.6f, 1, 255, 80, 40, 255); }
        } else if (c == 'X' && level_no == L_ENCORE) {   /* fly loft: a sandbag on a rope */
            float b = crusher_bottom(tx) + oy, cx = x + 24, ph = crush_phase(tx);
            draw_bar(cx, y, cx, b - 70, 3, 150, 120, 80, 255);
            draw_ellipse(&A.disc, cx, b - 36, 46, 38, 40, 28, 16, 255); draw_ellipse(&A.disc, cx, b - 38, 42, 34, 160, 128, 80, 255);
            draw_bar(cx - 14, b - 72, cx + 14, b - 72, 6, 120, 90, 50, 255);
            draw_text_col(&A.small, "50", cx, b - 50, 0.5f, 1, 90, 60, 30, 255);
            if (lit[zone_of(tx * TILE)] <= 0 && ph > 1.6f && ph < 2.0f) { float k = (ph - 1.6f) / 0.4f; draw_ellipse(&A.disc, cx, FLOOR_Y + oy, 46 * (0.6f + 0.4f * k), 9, 0, 0, 0, (Uint8)(80 + 140 * k));
                if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", cx, FLOOR_Y + oy - 70, 0.7f, 1, 255, 80, 40, 255); }
        } else if (c == 'X') {   /* ceiling crusher: shaft + riveted iron head */
            float b = crusher_bottom(tx) + oy, cx = x + 24, ph = crush_phase(tx);
            fill_rect((int)cx - 10, (int)y, 20, (int)(b - 40 - y), 70, 60, 50, 255); fill_rect((int)cx - 4, (int)y, 4, (int)(b - 40 - y), 150, 130, 100, 255);
            fill_rect((int)cx - 50, (int)b - 44, 100, 44, 30, 24, 20, 255); fill_rect((int)cx - 46, (int)b - 40, 92, 36, 96, 84, 70, 255);
            fill_rect((int)cx - 46, (int)b - 12, 92, 8, 60, 50, 40, 255);
            for (int k = 0; k < 4; k++) draw_ellipse(&A.disc, cx - 34 + k * 23, b - 30, 4, 4, 200, 170, 110, 255);
            if (lit[zone_of(tx * TILE)] <= 0 && ph > 1.6f && ph < 2.0f) {   /* warning shadow on the floor */
                float k = (ph - 1.6f) / 0.4f; draw_ellipse(&A.disc, cx, FLOOR_Y + oy, 50 * (0.6f + 0.4f * k), 9, 0, 0, 0, (Uint8)(80 + 140 * k));
                if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", cx, FLOOR_Y + oy - 70, 0.7f, 1, 255, 80, 40, 255); }
            if (lit[zone_of(tx * TILE)] > 0) draw_text_col(&A.small, "OFF", cx, b + 6, 0.4f, 1, 160, 220, 255, 255);
        } else if (c == 'V') {   /* steam vent grate */
            fill_rect((int)x + 2, (int)y + 34, TILE - 4, 14, 30, 26, 22, 255); fill_rect((int)x + 4, (int)y + 36, TILE - 8, 10, 120, 104, 80, 255);
            for (int k = 0; k < 5; k++) fill_rect((int)x + 7 + k * 8, (int)y + 37, 3, 8, 30, 26, 22, 255);
            if (vent_on(tx)) draw_glow(&A.disc, x + 24, y + 20, 40, 255, 255, 250, 140);
            else if (fmodf(scene_t + tx * 0.7f, 2.6f) > 2.3f && (frame_no / 4) % 2) draw_text_col(&A.small, "^", x + 24, y + 4, 0.6f, 1, 255, 255, 255, 200);
        } else if (c == 'S') {   /* sprocket saw */
            float cy = saw_y(tx, ty) + oy, cx = x + 24, a = frame_no * 0.35f;
            if (ty < 12) draw_bar(cx, ty * TILE + oy - 60, cx, cy, 3, 40, 38, 38, 255);
            for (int k = 0; k < 10; k++) { float b = a + k * 0.628f; draw_ellipse(&A.disc, cx + cosf(b) * 32, cy + sinf(b) * 32, 7, 7, 30, 28, 28, 255); }
            draw_ellipse(&A.disc, cx, cy, 33, 33, 30, 28, 28, 255); draw_ellipse(&A.disc, cx, cy, 28, 28, 190, 186, 178, 255);
            for (int k = 0; k < 10; k++) { float b = a + k * 0.628f; draw_ellipse(&A.disc, cx + cosf(b) * 32, cy + sinf(b) * 32, 4, 4, 225, 222, 214, 255); }
            for (int k = 0; k < 4; k++) { float b = -a * 0.5f + k * 1.571f; draw_ellipse(&A.disc, cx + cosf(b) * 15, cy + sinf(b) * 15, 6, 6, 30, 28, 28, 255); }
            draw_ellipse(&A.disc, cx, cy, 6, 6, 20, 18, 18, 255);
        } else if (c == '#') {
            int top = ty == 0 || map[ty - 1][tx] != '#';
            fill_rect((int)x, (int)y, TILE, TILE, 72, 44, 28, 255);
            fill_rect((int)x, (int)y + 15, TILE, 2, 40, 22, 14, 255);
            fill_rect((int)x, (int)y + 31, TILE, 2, 40, 22, 14, 255);
            fill_rect((int)x + ((tx * 7 + ty * 3) % 3) * 16, (int)y, 2, TILE, 44, 26, 16, 255);
            if (top) { fill_rect((int)x, (int)y, TILE, 6, 150, 104, 62, 255); fill_rect((int)x, (int)y + 6, TILE, 3, 20, 10, 6, 255);
                if ((tx * 13) % 7 == 0) draw_ellipse(&A.disc, x + 24, y + 9, 7, 11 + (tx % 3) * 4, 12, 8, 8, 255); }
        } else if (c == '=') {
            fill_rect((int)x, (int)y, TILE, 10, 58, 52, 50, 255);
            fill_rect((int)x, (int)y, TILE, 3, 150, 140, 120, 255);
            fill_rect((int)x + 4, (int)y + 10, 3, 14, 40, 36, 34, 255);
            draw_ellipse(&A.disc, x + 10, y + 5, 2, 2, 200, 180, 140, 255); draw_ellipse(&A.disc, x + 38, y + 5, 2, 2, 200, 180, 140, 255);
        } else if (c == '~' && level_no == 1) {   /* hot cheese */
            fill_rect((int)x, (int)y, TILE, TILE, 200, 120, 30, 255);
            if (ty == 13) { float w = sinf(frame_no * 0.06f + tx * 0.9f) * 4;
                draw_ellipse(&A.disc, x + 24, y + 4 + w, 30, 10, 245, 185, 60, 255);
                draw_ellipse(&A.disc, x + 16, y + 2 + w, 7, 3, 255, 240, 170, 200);
                if ((frame_no + tx * 13) % 50 < 12) draw_ellipse(&A.disc, x + 30, y - 2 + w, 5 + (frame_no % 12) * 0.4f, 5, 255, 210, 90, 255); }
        } else if (c == '~') {
            fill_rect((int)x, (int)y, TILE, TILE, 8, 6, 8, 255);
            if (ty == 13) { float w = sinf(frame_no * 0.06f + tx * 0.9f) * 4;
                draw_ellipse(&A.disc, x + 24, y + 4 + w, 30, 10, 14, 10, 14, 255);
                draw_ellipse(&A.disc, x + 16, y + 2 + w, 6, 2, 90, 80, 90, 120); }
        } else if (c == 'O') {   /* brick oven */
            fill_rect((int)x, (int)y, TILE, TILE, 120, 52, 34, 255);
            for (int r = 0; r < 3; r++) fill_rect((int)x, (int)y + r * 16 + 15, TILE, 2, 60, 26, 16, 255);
            fill_rect((int)x + ((tx + ty) % 2) * 24, (int)y, 2, TILE, 60, 26, 16, 255);
            if (map[ty - 1][tx] != 'O') { fill_rect((int)x, (int)y, TILE, 6, 160, 80, 50, 255);
                int left = map[ty][tx - 1] != 'O';
                if (left) { float ph = oven_phase(tx); int on = oven_burning(tx);
                    fill_rect((int)x + 10, (int)y + 20, 76, 30, 20, 8, 4, 255);
                    draw_glow(&A.disc, x + 48, y + 34, on ? 60 : (ph > 2.9f ? 40 : 20), 255, 130, 30, on ? 255 : 160);
                    if (on) {   /* the fire column */
                        float k = fminf(1, ph / 0.15f) * fminf(1, (1.1f - ph) / 0.2f), hgt = 170 * k;
                        draw_ellipse(&A.disc, x + 48, y - hgt * 0.5f, 34, hgt * 0.55f, 255, 120, 30, 220);
                        draw_ellipse(&A.disc, x + 48, y - hgt * 0.45f, 20, hgt * 0.45f, 255, 220, 120, 255);
                        draw_glow(&A.disc, x + 48, y - hgt * 0.5f, 80, 255, 140, 40, 200);
                    } else if (ph > 2.9f && (frame_no / 4) % 2) draw_text_col(&A.small, "!", x + 48, y - 40, 0.7f, 1, 255, 120, 40, 255);
                    if (lit[zone_of(tx * TILE)] > 0) draw_text_col(&A.small, "OFF", x + 48, y - 24, 0.4f, 1, 160, 220, 255, 255);
                } }
        } else if (c == '>' || c == '<') {   /* conveyor belt */
            fill_rect((int)x, (int)y, TILE, 20, 50, 46, 44, 255);
            fill_rect((int)x, (int)y + 20, TILE, TILE - 20, 30, 26, 24, 255);
            int dir = c == '>' ? 1 : -1, off = ((frame_no * 3 * dir) % 16 + 16) % 16;
            for (int k = -1; k < 4; k++) fill_rect((int)x + k * 16 + off, (int)y + 3, 8, 3, 150, 140, 120, 255);
            draw_ellipse(&A.disc, x + 12, y + 30, 8, 8, 90, 84, 80, 255); draw_ellipse(&A.disc, x + 36, y + 30, 8, 8, 90, 84, 80, 255);
        } else if (c == 'B') {
            int used = 0; for (int i = 0; i < nbreak; i++) if (breakers[i].tx == tx) used = breakers[i].used;
            fill_rect((int)x + 6, (int)y + 2, 36, 50, 30, 28, 26, 255); fill_rect((int)x + 9, (int)y + 5, 30, 44, 110, 104, 92, 255);
            fill_rect((int)x + 20, (int)y + (used ? 10 : 28), 8, 14, 40, 30, 20, 255);
            Uint8 on = used ? 1 : (frame_no / 20) % 2;
            draw_ellipse(&A.disc, x + 24, y + 44, 4, 4, used ? 80 : 255, used ? 255 : 60, 60, 255);
            if (!used && on) draw_glow(&A.disc, x + 24, y + 44, 18, 255, 60, 40, 200);
            if (!used) draw_text_col(&A.small, "UP", x + 24, y - 22, 0.45f, 1, 255, 220, 160, 200);
        } else if (c == 'C' && ty == 11) {
            fill_rect((int)x + 2, (int)y - 6, 44, 102, 30, 18, 12, 255); fill_rect((int)x + 6, (int)y - 2, 36, 94, 110, 66, 38, 255);
            fill_rect((int)x + 10, (int)y + 2, 28, 40, 92, 54, 30, 255); fill_rect((int)x + 10, (int)y + 48, 28, 38, 92, 54, 30, 255);
            draw_ellipse(&A.disc, x + 36, y + 46, 3, 3, 220, 190, 90, 255);
            draw_text_col(&A.small, "PROPS", x + 24, y + 12, 0.32f, 1, 240, 220, 170, 255);
        } else if (c == 'K') {
            int on = 0; for (int i = 0; i <= cur_check; i++) if (checkpoints[i] == tx) on = 1;
            fill_rect((int)x + 21, (int)y - 50, 6, 98, 40, 34, 30, 255);
            draw_ellipse(&A.disc, x + 24, y - 56, 12, 14, on ? 255 : 90, on ? 236 : 80, on ? 170 : 70, 255);
            if (on) draw_glow(&A.disc, x + 24, y - 56, 60 + sinf(frame_no * 0.1f) * 4, 255, 210, 120, 180);
        }
    }
}

static void draw_sprite(Tex *t, float x, float feet, float scale, int face_right, Uint8 flash, Uint8 alpha, double angle, float sy) {
    if (!t->tex) return;
    float w = t->w * scale, h = t->h * scale * sy;
    Uint8 c = 255;
    draw_tex_tint(t, x - w / 2, feet - h, w, h, !face_right, c, c, c, alpha, angle);
    if (flash) { SDL_SetTextureBlendMode(t->tex, SDL_BLENDMODE_ADD); draw_tex_tint(t, x - w / 2, feet - h, w, h, !face_right, 255, 255, 255, flash, angle); SDL_SetTextureBlendMode(t->tex, SDL_BLENDMODE_BLEND); }
}

static void draw_enemy(Enemy *e, float ox, float oy) {
    float x = sx(e->x) + ox, y = e->y + oy;
    Uint8 fl = e->flash ? 200 : 0; Tex *et = enemy_tex(e);
    switch (e->type) {
    case E_USHER: {
        draw_sprite(et, x, y + 2, 1, e->face > 0, fl, 255, sinf(e->t * 14) * (fabsf(e->vx) > 5 ? 3 : 0), 1);
        if (e->state == 1) draw_glow(&A.disc, x + e->face * 44, y - 98, 40 + (0.5f - e->timer) * 40, 255, 240, 170, 230);
        break; }
    case E_DOG: {
        float sq = e->state == 1 ? 1.0f - 0.06f * sinf(e->t * 40) : 1;
        draw_sprite(et, x, y + 2, 1, e->face > 0, fl, 255, 0, sq);
        float lx = x + e->face * 50, ly = y - 76;
        if (e->state == 1) draw_glow(&A.disc, lx, ly, 20 + (0.9f - e->timer) * 50, 255, 250, 200, 255);
        if (e->state == 2) {
            float x1 = lx + e->face * 700;
            draw_bar(lx, ly, x1, ly, 34 + sinf(frame_no) * 4, 255, 236, 170, 120);
            draw_bar(lx, ly, x1, ly, 12, 255, 255, 240, 230);
            draw_glow(&A.disc, lx, ly, 50, 255, 250, 220, 255);
        }
        break; }
    case E_BLOB: case E_MINIBLOB: {
        float s = e->type == E_MINIBLOB ? 0.6f : 1.0f;
        float squash = e->onground ? 1.0f + 0.05f * sinf(e->t * 8) : 1.0f;
        draw_sprite(et, x, y + 2, s, e->face > 0, fl, 255, 0, squash);
        break; }
    case E_CRATE: case E_HATCRATE: {
        float w = 52, h = 46, bx = x - w / 2, by = y - h;
        fill_rect((int)bx, (int)by, (int)w, (int)h, 30, 18, 10, 255);
        fill_rect((int)bx + 3, (int)by + 3, (int)w - 6, (int)h - 6, e->type == E_HATCRATE ? 200 : 168, e->type == E_HATCRATE ? 160 : 112, e->type == E_HATCRATE ? 40 : 60, 255);
        draw_bar(bx + 4, by + 4, bx + w - 4, by + h - 4, 4, 90, 56, 28, 255);
        draw_bar(bx + w - 4, by + 4, bx + 4, by + h - 4, 4, 90, 56, 28, 255);
        draw_text_col(&A.small, e->type == E_HATCRATE ? "GEAR" : "TOOLS", x, by - 22, 0.36f, 1, 255, 230, 170, 220);
        if (e->flash) fill_rect((int)bx, (int)by, (int)w, (int)h, 255, 255, 255, 120);
        break; }
    case E_GHOST: {
        int faded = ghost_faded(e); Uint8 al = faded ? (Uint8)(40 + 30 * ((frame_no / 3) % 2)) : 235;
        draw_sprite(et, x, y + 2, 1, e->face > 0, fl, al, sinf(e->t * 3) * 4, 1);
        break; }
    case E_BOMB: {
        float sw = e->state == 1 ? 1.0f + (0.8f - e->timer) * 0.35f : 1.0f;
        if (e->state == 1 && (frame_no / 3) % 2) fl = 170;
        draw_sprite(et, x, y + 2, sw, e->face > 0, fl, 255, 0, e->land > 0 ? 0.85f : 1.0f);
        break; }
    case E_EEL: {
        if (e->state == 0) { draw_ellipse(&A.disc, x, y - 2, 26, 6, 30, 26, 30, 200); break; }
        draw_sprite(et, x, y + 14, 1, e->face > 0, fl, 255, 0, 1);
        draw_ellipse(&A.disc, x, y - 2, 40, 9, 10, 8, 10, 255);   /* the splash ring it rises out of */
        break; }
    case E_MIRROR: {   /* the hero's own sprite, silvered like a reflection */
        if (!et || !et->tex) break;
        float w = et->w, h = et->h;
        draw_tex_tint(et, x - w / 2, y + 2 - h, w, h, e->face < 0, 170, 200, 235, 215, 0);
        if (e->flash) draw_tex_tint(et, x - w / 2, y + 2 - h, w, h, e->face < 0, 255, 255, 255, 140, 0);
        { float gx = x - w / 2 + fmodf(scene_t * 90 + e->mx, w + 40) - 20; draw_bar(gx, y - h, gx - 20, y, 6, 255, 255, 255, 60); }
        break; }
    case E_CROW:   /* a pale rim so the black crows read against the night sky */
        draw_glow(&A.disc, x, y - 26, 46, 150, 170, 210, 70);
        draw_sprite(et, x, y + 2, 1, e->face > 0, fl, 255, 0, 1);
        break;
    case E_BULB: {
        draw_sprite(et, x, y + 2, 1, e->face > 0, fl, 255, 0, 1);
        draw_glow(&A.disc, x, y - 74, e->state == 1 ? 60 + sinf(frame_no * 0.9f) * 14 : 30, 255, 230, 120, e->state == 1 ? 230 : 90);
        break; }
    case E_SHADOW: {   /* the hero's own sprite, drawn as living ink with white eyes */
        if (!et || !et->tex) break;
        float w = et->w, h = et->h; draw_tex_tint(et, x - w / 2, y + 2 - h, w, h, e->face < 0, 18, 12, 22, 245, 0);
        if (e->flash) draw_tex_tint(et, x - w / 2, y + 2 - h, w, h, e->face < 0, 255, 255, 255, 120, 0);
        draw_ellipse(&A.disc, x + e->face * 6 - 6, y - 118, 4, 5, 255, 255, 255, 255); draw_ellipse(&A.disc, x + e->face * 6 + 6, y - 118, 4, 5, 255, 255, 255, 255);
        if (frame_no % 4 == 0) part_k(PK_BLOOD, e->x + (frand() - 0.5f) * 30, e->y - frand() * 120, 0, 40, 0.8f, 3, 900, 12, 8, 12, 0);
        break; }
    default:   /* goats, meatballs, slices, gloves, imps, workers */
        draw_sprite(et, x, y + 2, 1, e->face > 0, fl, 255, 0, 1);
        break;
    }
    if (e->marked > 0) {   /* pinged by the security office */
        float r = 50 + sinf(frame_no * 0.2f) * 6, cy = y - 50;
        draw_bar(x - r, cy, x - r + 18, cy, 4, 255, 50, 40, 230); draw_bar(x + r - 18, cy, x + r, cy, 4, 255, 50, 40, 230);
        draw_bar(x, cy - r, x, cy - r + 18, 4, 255, 50, 40, 230); draw_bar(x, cy + r - 18, x, cy + r, 4, 255, 50, 40, 230);
    }
    if (e->burn > 0 && et && et->tex) {   /* on fire: flickering orange glow over the body */
        float w = et->w, h = et->h * (e->type == E_MINIBLOB ? 0.6f : 1.0f);
        draw_glow(&A.disc, x, y - h * 0.5f, h * 0.7f, 255, 120, 30, (Uint8)(120 + 60 * sinf(frame_no * 0.7f)));
        (void)w;
    }
    if (e->stun > 0) for (int k = 0; k < 3; k++) { float a = frame_no * 0.15f + k * 2.1f; draw_ellipse(&A.disc, x + cosf(a) * 26, y - 140 + sinf(a) * 8, 5, 5, 255, 240, 140, 255); }
}

static void draw_player(Player *p, float ox, float oy) {
    if (!p->active) return;
    if (p->hidden) {
        float x = sx(p->closet_tx * TILE + 24) + ox;
        draw_ellipse(&A.disc, x - 6, 11 * TILE + 26 + oy, 3, 2, 255, 255, 255, 230); draw_ellipse(&A.disc, x + 6, 11 * TILE + 26 + oy, 3, 2, 255, 255, 255, 230);
        return;
    }
    if (p->inv > 0 && p->alive && ((int)(p->inv * 20)) % 2) return;
    int set = p->gear >= 3 ? 0 : p->gear == 2 ? 1 : 2;
    int fr = 0; float sy = 1;
    if (!p->alive) fr = 5;
    else if (!p->onground) fr = p->aim_dy < 0 ? (p->aim_dx ? 6 : 7) : 5;
    else if (p->aim_dy < 0) fr = p->aim_dx ? 6 : 7;
    else if (fabsf(p->vx) > 20) fr = 1 + ((int)(p->anim * 8)) % 4;
    else fr = 0;
    if (p->crouch) sy = 0.68f;
    else if (p->onground && fabsf(p->vx) < 20) sy = 1.0f + 0.015f * sinf(p->bob * 4);
    double ang = p->alive ? 0 : p->dead_t * 600;
    float x = sx(p->x) + ox, y = p->y + oy + 4;
    draw_ellipse(&A.disc, x, FLOOR_Y + oy - 2 > y ? y - 2 : y - 2, 34, 7, 0, 0, 0, p->onground ? 90 : 40);
    { Tex *t = &A.hero[p->chr][set][fr];
      float sxk = 1; int face_r = p->face > 0;
      if (p->land_t > 0) { float k = p->land_t / 0.14f; sy *= 1 - 0.2f * k; sxk *= 1 + 0.22f * k; }
      if (p->jump_t > 0) { float k = p->jump_t / 0.16f; sy *= 1 + 0.16f * k; sxk *= 1 - 0.13f * k; }
      if (p->turn_t > 0) { float k = p->turn_t / 0.1f; sxk *= 0.35f + 0.65f * fabsf(cosf(k * 3.14159f)); }
      if (p->idle_t > 5 && fmodf(p->idle_t - 5, 4.0f) > 3.0f) face_r = !face_r;   /* standing still: glance over the shoulder now and then */
      Uint8 r = OUTFIT_RGB[save.outfit][0], g = OUTFIT_RGB[save.outfit][1], b = OUTFIT_RGB[save.outfit][2];
      if (p->hurt_t > 0 && p->alive) { float k = p->hurt_t / 0.45f; ang += -p->face * 12 * k; g = (Uint8)(g * (1 - 0.5f * k)); b = (Uint8)(b * (1 - 0.5f * k)); sy *= 1 - 0.08f * k; }
      if (t->tex) { float w = t->w * sxk, hgt = t->h * sy; draw_tex_tint(t, x - w / 2, y - hgt, w, hgt, !face_r, r, g, b, 255, ang); } }
    if ((p->hurt_t > 0 && p->alive) || (!p->alive && p->dead_t < 1.2f)) {   /* cartoon stars circling the head */
        float hy = p->alive ? y - 150 : y - 110;
        for (int k = 0; k < 3; k++) { float a = scene_t * 7 + k * 2.094f, sxp = x + cosf(a) * 34, syp = hy + sinf(a) * 9;
            draw_glow(&A.disc, sxp, syp, 12, 255, 220, 90, 200); draw_bar(sxp - 7, syp, sxp + 7, syp, 3, 255, 240, 140, 255); draw_bar(sxp, syp - 7, sxp, syp + 7, 3, 255, 240, 140, 255); } }
    if (p->shoot_t > 0.08f) {
        float mx = x + p->face * 52, my = y - 82;
        if (p->aim_dy < 0 && p->aim_dx) { mx = x + p->face * 50; my = y - 130; } else if (p->aim_dy < 0) { mx = x + p->face * 18; my = y - 160; }
        else if (p->crouch) my = y - 50;
        draw_glow(&A.disc, mx, my, 22, 255, 220, 140, 255);
    }
    if (p == &pl[1]) draw_text_col(&A.small, "P2", x, y - 178, 0.4f, 1, 120, 200, 255, 255);
}

static void draw_proj(float ox, float oy) {
    for (int i = 0; i < MAXP; i++) { Proj *p = &pr[i]; if (!p->alive) continue;
        float x = sx(p->x) + ox, y = p->y + oy;
        switch (p->type) {
        case P_NAIL: { float a = atan2f(p->vy, p->vx); float ex = x - cosf(a) * 22, ey = y - sinf(a) * 22;
            draw_bar(ex, ey, x, y, 4, 200, 200, 210, 255); draw_bar(ex - cosf(a) * 14, ey - sinf(a) * 14, ex, ey, 2, 255, 230, 160, 160);
            draw_ellipse(&A.disc, ex, ey, 4, 4, 160, 160, 170, 255); break; }
        case P_RIVET: draw_glow(&A.disc, x, y, 22, 255, 140, 40, 220); draw_ellipse(&A.disc, x, y, 7, 7, 255, 210, 120, 255); break;
        case P_FLAME: { float k = p->life / 0.24f; draw_glow(&A.disc, x, y, 34 * (1.4f - k), 120, 180, 255, (Uint8)(200 * k)); draw_ellipse(&A.disc, x, y, 10 * k + 4, 10 * k + 4, 230, 245, 255, (Uint8)(255 * k)); break; }
        case P_PLUNGER: draw_bar(x - 14, y, x + 6, y, 5, 140, 90, 50, 255); draw_ellipse(&A.disc, x + 10, y, 9, 12, 200, 40, 40, 255); break;
        case P_LIGHT: draw_glow(&A.disc, x, y, 36, 255, 240, 170, 255); draw_ellipse(&A.disc, x, y, 9, 9, 255, 255, 230, 255); break;
        case P_NOTE: draw_ellipse(&A.disc, x, y, 16, 13, 10, 8, 10, 255); draw_bar(x + 13, y, x + 13, y - 44, 5, 10, 8, 10, 255); draw_bar(x + 13, y - 44, x + 30, y - 32, 6, 10, 8, 10, 255);
            draw_ellipse(&A.disc, x - 5, y - 4, 4, 3, 255, 255, 255, 200); break;
        case P_TICKET: draw_tex_tint(&A.disc, x - 18, y - 9, 36, 18, 0, 0, 0, 0, 0, 0);
            fill_rect((int)x - 18, (int)y - 9, 36, 18, 220, 60, 50, 255); fill_rect((int)x - 14, (int)y - 5, 28, 10, 250, 220, 160, 255);
            fill_rect((int)x - 18, FLOOR_Y + (int)oy - 4, 36, 4, 0, 0, 0, 90); break;
        case P_CONFETTI: { Uint8 r = (Uint8)(120 + (i * 37) % 136), g = (Uint8)(80 + (i * 71) % 176), b = (Uint8)(80 + (i * 113) % 176);
            draw_ellipse(&A.disc, x, y, 12, 12, 30, 20, 20, 255); draw_ellipse(&A.disc, x, y, 9, 9, r, g, b, 255);
            draw_bar(x, y - 9, x + 6, y - 18, 3, 230, 210, 160, 255); if (frame_no % 4 < 2) draw_glow(&A.disc, x + 6, y - 18, 10, 255, 200, 80, 255); break; }
        case P_PIZZA: { float a = frame_no * 0.4f * (p->vx > 0 ? 1 : -1);
            draw_ellipse(&A.disc, x, y, 28, 28, 120, 60, 20, 255); draw_ellipse(&A.disc, x, y, 23, 23, 235, 180, 70, 255);
            for (int k = 0; k < 5; k++) { float b = a + k * 1.256f; draw_ellipse(&A.disc, x + cosf(b) * 13, y + sinf(b) * 13, 5, 5, 180, 30, 20, 255); }
            draw_bar(x + cosf(a) * 22, y + sinf(a) * 22, x - cosf(a) * 22, y - sinf(a) * 22, 2, 150, 90, 40, 200); break; }
        case P_FILM: { float a = atan2f(p->vy, p->vx), ca = cosf(a), sa = sinf(a), wv = sinf(frame_no * 0.5f) * 6;
            draw_bar(x - ca * 34 - sa * wv, y - sa * 34 + ca * wv, x + ca * 10, y + sa * 10, 16, 20, 18, 18, 255);
            draw_bar(x - ca * 32 - sa * wv, y - sa * 32 + ca * wv, x + ca * 8, y + sa * 8, 9, 190, 186, 176, 255);
            for (int k = 0; k < 3; k++) draw_ellipse(&A.disc, x - ca * (k * 13) - sa * 7, y - sa * (k * 13) + ca * 7, 2, 2, 250, 246, 236, 255);
            break; }
        case P_REEL: { float a = p->x * 0.04f; float cy = y - 30 + 1;
            draw_ellipse(&A.disc, x, cy, 32, 32, 20, 18, 18, 255); draw_ellipse(&A.disc, x, cy, 27, 27, 150, 146, 140, 255);
            draw_ellipse(&A.disc, x, cy, 18, 18, 60, 58, 56, 255);
            for (int k = 0; k < 5; k++) { float b = a + k * 1.2566f; draw_ellipse(&A.disc, x + cosf(b) * 16, cy + sinf(b) * 16, 6, 6, 20, 18, 18, 255); }
            draw_ellipse(&A.disc, x, cy, 5, 5, 220, 216, 206, 255);
            fill_rect((int)x - 26, FLOOR_Y + (int)oy - 4, 52, 4, 0, 0, 0, 90); break; }
        case P_LIPSTICK: { float a = atan2f(p->vy, p->vx), c = cosf(a), sn = sinf(a);
            draw_glow(&A.disc, x, y, 24, 255, 70, 90, 150);
            draw_bar(x - c * 18, y - sn * 18, x + c * 4, y + sn * 4, 13, 210, 170, 60, 255);   /* gold tube */
            draw_bar(x + c * 2, y + sn * 2, x + c * 16, y + sn * 16, 10, 220, 30, 50, 255); break; }   /* red tip */
        case P_SHARD: { draw_bar(x, y - 16, x, y + 16, 9, 200, 225, 245, 255); draw_bar(x - 2, y - 12, x - 2, y + 6, 2, 255, 255, 255, 230);
            if (p->bounces > 0) draw_glow(&A.disc, x, y, 26, 220, 240, 255, 120); break; }
        case P_KISS: { float b = 1 + 0.12f * sinf(frame_no * 0.5f);
            draw_glow(&A.disc, x, y, 26, 255, 90, 150, 140);
            draw_ellipse(&A.disc, x - 6 * b, y - 3, 8 * b, 8 * b, 240, 40, 90, 255); draw_ellipse(&A.disc, x + 6 * b, y - 3, 8 * b, 8 * b, 240, 40, 90, 255);
            draw_bar(x - 12 * b, y, x, y + 12 * b, 7 * b, 240, 40, 90, 255); draw_bar(x + 12 * b, y, x, y + 12 * b, 7 * b, 240, 40, 90, 255); break; }
        case P_SPARK: { draw_glow(&A.disc, x, y, 30, 255, 230, 120, 200); float d = p->vx > 0 ? 1 : -1;
            for (int k = 0; k < 3; k++) draw_bar(x - d * (k * 10), y + (k % 2 ? -8 : 8), x - d * (k * 10 + 10), y + (k % 2 ? 8 : -8), 4, 255, 255, 210, 255); break; }
        case P_LETTER: { char b[2] = {(char)(p->last_hit > 0 ? p->last_hit : 'M'), 0}; int pink = (b[0] % 2) == 0;
            draw_glow(&A.disc, x, y, 40, pink ? 255 : 90, pink ? 90 : 220, pink ? 190 : 255, 160);
            draw_text_col(&A.font, b, x, y - 22, 1.1f, 1, pink ? 255 : 180, pink ? 160 : 240, pink ? 220 : 255, 255); break; }
        case P_BULB: { draw_glow(&A.disc, x, y, 30, 255, 220, 110, 200); draw_ellipse(&A.disc, x, y - 2, 12, 12, 255, 245, 190, 255);
            draw_bar(x - 6, y + 10, x + 6, y + 10, 6, 150, 120, 60, 255); break; }
        case P_CORK: draw_ellipse(&A.disc, x, y, 11, 8, 60, 36, 18, 255); draw_ellipse(&A.disc, x, y, 9, 6, 200, 150, 90, 255);
            draw_ellipse(&A.disc, x - p->vx * 0.01f, y, 3, 5, 150, 100, 60, 255); break;
        case P_SHOE: { float a = frame_no * 0.3f * (p->vx > 0 ? 1 : -1), ca = cosf(a), sa = sinf(a);
            draw_bar(x - ca * 18, y - sa * 18, x + ca * 18, y + sa * 18, 22, 30, 18, 10, 255); draw_bar(x - ca * 16, y - sa * 16, x + ca * 14, y + sa * 14, 16, 120, 70, 40, 255);
            draw_ellipse(&A.disc, x - ca * 10 - sa * 8, y - sa * 10 + ca * 8, 8, 8, 30, 18, 10, 255); break; }
        case P_INKBLOB: draw_ellipse(&A.disc, x, y, 14, 12, 10, 8, 10, 255); draw_ellipse(&A.disc, x - 4, y - 4, 4, 3, 160, 160, 170, 200);
            draw_ellipse(&A.disc, x - p->vx * 0.02f, y - p->vy * 0.02f, 8, 7, 10, 8, 10, 255); break;
        case P_SHEET: { float a = frame_no * 0.5f + i, ca = cosf(a) * 26, sa = sinf(a) * 26, cb = -sinf(a) * 18, sb = cosf(a) * 18;
            draw_bar(x - ca, y - sa, x + ca, y + sa, 36, 30, 26, 22, 255); draw_bar(x - ca, y - sa, x + ca, y + sa, 30, 238, 228, 205, 255);
            for (int k = -1; k <= 1; k++) draw_bar(x - ca * 0.7f + cb * k * 0.4f, y - sa * 0.7f + sb * k * 0.4f, x + ca * 0.7f + cb * k * 0.4f, y + sa * 0.7f + sb * k * 0.4f, 2, 40, 34, 30, 255);
            break; }
        case P_DRIP: draw_ellipse(&A.disc, x, y, 7, 11, 10, 8, 10, 255); draw_ellipse(&A.disc, x - 2, y - 3, 2, 3, 200, 200, 200, 160); break;
        }
    }
    for (int i = 0; i < 8; i++) { Pickup *k = &pick[i]; if (!k->alive) continue;
        if (k->t > 11 && ((int)(k->t * 10)) % 2) continue;
        float x = sx(k->x) + ox, y = k->y + oy - 26 + sinf(k->t * 5) * 4;
        draw_glow(&A.disc, x, y, 40, 255, 220, 120, 160);
        fill_rect((int)x - 22, (int)y - 22, 44, 44, 30, 20, 14, 255); fill_rect((int)x - 19, (int)y - 19, 38, 38, 240, 220, 170, 255);
        if (k->kind == 10) { draw_ellipse(&A.disc, x, y + 2, 15, 12, 230, 180, 50, 255); fill_rect((int)x - 18, (int)y + 4, 36, 5, 200, 150, 40, 255); }
        else { int w = ((int)(k->t / 0.6f)) % NWEAP; const char *L[4] = {"N", "S", "T", "P"};
            draw_text_col(&A.font, L[w], x, y - 20, 0.8f, 1, 120, 40, 30, 255); }
    }
}

static void draw_splats(float ox, float oy) {
    for (int i = 0; i < MAXSPLAT; i++) { Splat *d = &splats[i]; if (d->a == 0) continue;
        float x = sx(d->x) + ox, y = d->y + oy; if (x < -120 || x > SCREEN_W + 120) continue;
        if (d->tex >= 0) draw_tex_srcx(&A.gore[d->tex], NULL, x - d->rx, y - d->ry, d->rx * 2, d->ry * 2, i & 1, d->ang, d->r > 60 ? 255 : 90, d->r > 60 ? 255 : 80, d->r > 60 ? 255 : 90, d->a);
        else draw_ellipse(&A.disc, x, y, d->rx, d->ry, d->r, d->g, d->b, d->a);
    }
}
static void draw_gibs(float ox, float oy) {
    for (int i = 0; i < MAXGIB; i++) { Gib *g = &gibs[i]; if (g->life <= 0) continue;
        Uint8 a = (Uint8)(255 * fminf(1, g->life));
        draw_tex_srcx(g->t, &g->src, sx(g->x) + ox - g->w / 2, g->y + oy - g->h / 2, g->w, g->h, g->flip, g->ang, 255, 255, 255, a);
    }
}
static void draw_parts(float ox, float oy) {
    for (int i = 0; i < 16; i++) { Ring *r = &rings[i]; if (r->life <= 0) continue;
        float k = r->life / 0.45f, x = sx(r->x) + ox, y = r->y + oy;
        for (int a = 0; a < 28; a++) { float t = a * 6.2832f / 28; draw_ellipse(&A.disc, x + cosf(t) * r->r, y + sinf(t) * r->r * 0.45f, 6 * k + 2, 6 * k + 2, r->cr, r->cg, r->cb, (Uint8)(220 * k)); }
    }
    for (int i = 0; i < MAXPART; i++) { Part *q = &parts[i]; if (q->life <= 0) continue;
        float k = q->life / q->max, s = q->size * (0.5f + 0.5f * k);
        if (q->kind == PK_SMOKE) { draw_ellipse(&A.disc, sx(q->x) + ox, q->y + oy, q->size, q->size, q->r, q->g, q->b, (Uint8)(110 * k)); continue; }
        if (q->kind == PK_CASING) { float a = q->ang * 0.01745f; draw_bar(sx(q->x) + ox - cosf(a) * 5, q->y + oy - sinf(a) * 5, sx(q->x) + ox + cosf(a) * 5, q->y + oy + sinf(a) * 5, 4, q->r, q->g, q->b, (Uint8)(255 * fminf(1, k * 3))); continue; }
        if (q->kind == PK_DEBRIS) { float a = q->ang * 0.01745f, L = q->size; draw_bar(sx(q->x) + ox - cosf(a) * L, q->y + oy - sinf(a) * L, sx(q->x) + ox + cosf(a) * L, q->y + oy + sinf(a) * L, L * 0.5f, q->r, q->g, q->b, (Uint8)(255 * fminf(1, k * 3))); continue; }
        if (q->add) draw_glow(&A.disc, sx(q->x) + ox, q->y + oy, s * 2.5f, q->r, q->g, q->b, (Uint8)(255 * k));
        else draw_ellipse(&A.disc, sx(q->x) + ox, q->y + oy, s, s, q->r, q->g, q->b, (Uint8)(255 * fminf(1, k * 2))); }
}

static void draw_fiend(float ox, float oy) {
    if (fiend.state < 2) return;
    float x = sx(fiend.x) + ox, y = fiend.y + oy;
    Tex *t = &A.blob[(frame_no / 10) & 1];
    float h = 300 + sinf(frame_no * 0.12f) * 14;
    draw_glow(&A.disc, x, y - h * 0.5f, 170, 90, 0, 0, 140);
    if (t->tex) { float w = t->w * 2.7f; draw_tex_tint(t, x - w / 2, y - h, w, h, fiend.x > (pl[0].x), 255, 255, 255, 255, sinf(frame_no * 0.08f) * 3); }
    for (int k = 0; k < 5; k++) draw_ellipse(&A.disc, x - 90 + k * 45, y - 6, 26, 10, 8, 6, 8, 255);
}

static void draw_boss_rat(float ox, float oy);
static void draw_boss_splicer(float ox, float oy);
static void draw_boss_press(float ox, float oy);
static void draw_boss_collector(float ox, float oy);
static void draw_boss_vanity(float ox, float oy);
static void draw_boss_headliner(float ox, float oy);
static void draw_boss_unbound(float ox, float oy);
static void draw_boss_inner(float ox, float oy) {
    if (!boss.active) return;
    if (level_no == L_ENCORE) { draw_boss_unbound(ox, oy); return; }
    if (level_no == L_ROOF) { draw_boss_headliner(ox, oy); return; }
    if (level_no == L_MIRROR) { draw_boss_vanity(ox, oy); return; }
    if (level_no == 4) { draw_boss_collector(ox, oy); return; }
    if (level_no == 3) { draw_boss_press(ox, oy); return; }
    if (level_no == 2) { draw_boss_splicer(ox, oy); return; }
    if (level_no == 1) { draw_boss_rat(ox, oy); return; }
    float x = sx(boss.x) + ox, y = boss.y + oy + boss.bob;
    if (boss.state == BS_SLAM_AIM) {   /* ink shadow where the tentacle will land */
        float k = 1 - boss.timer / 0.85f; float tx = sx(boss.targetx) + ox;
        draw_ellipse(&A.disc, tx, FLOOR_Y + oy, 30 + 30 * k, 8 + 6 * k, 10, 6, 10, (Uint8)(120 + 120 * k));
        if ((frame_no / 4) % 2) draw_text_col(&A.small, "!", tx, FLOOR_Y + oy - 60, 0.8f, 1, 255, 60, 40, 255);
    }
    if (boss.state == BS_SLAM_HIT) {   /* giant gloved tentacle crashes down */
        float tx = sx(boss.targetx) + ox;
        draw_bar(tx, -20, tx, FLOOR_Y + oy - 30, 70, 120, 100, 120, 255);
        draw_bar(tx - 22, -20, tx - 22, FLOOR_Y + oy - 30, 10, 80, 64, 86, 255);
        for (int k = 0; k < 6; k++) draw_ellipse(&A.disc, tx + 18, 60 + k * 90 + oy, 10, 10, 180, 150, 170, 255);
        draw_ellipse(&A.disc, tx, FLOOR_Y + oy - 34, 50, 40, 250, 248, 236, 255);
        draw_ellipse(&A.disc, tx, FLOOR_Y + oy - 34, 50, 40, 20, 20, 20, 0);
    }
    float hx = x + boss.face * 140;
    if (boss.state == BS_SWEEP_WARN && (frame_no / 5) % 2) draw_glow(&A.disc, hx, y - 110, 70, 255, 240, 170, 255);
    if (boss.state == BS_SWEEP) {
        float bx = sx(boss.sweepx) + ox;
        draw_bar(hx, y - 110, bx, FLOOR_Y + oy - 26, 50, 255, 240, 170, 110);
        draw_ellipse(&A.disc, bx, FLOOR_Y + oy - 26, 50, 26, 255, 250, 210, 220);
        draw_glow(&A.disc, bx, FLOOR_Y + oy - 26, 90, 255, 230, 150, 255);
    }
    if (boss.pose < 0) return;
    Tex *t = &A.inky[boss.pose];
    double ang = boss.state == BS_DEAD ? sinf(frame_no * 0.5f) * 6 : sinf(boss.t * 1.3f) * 2;
    float sy = 1.0f + 0.02f * sinf(boss.t * 4); Uint8 al = 255;
    if (boss.state == BS_WARP) {   /* melt down into a puddle, slide, pour back up */
        float k = boss.timer > 0.55f ? (boss.timer - 0.55f) / 0.45f : boss.timer < 0.45f ? 1 - boss.timer / 0.45f : 0;
        draw_ellipse(&A.disc, x, FLOOR_Y + oy - 6, 130 - 50 * k, 22 - 6 * k, 14, 10, 20, 230);
        if (k < 0.05f) return;
        sy = k; al = (Uint8)(120 + 135 * k);
    }
    draw_sprite(t, x, y + 10, 1, boss.face > 0, boss.flash ? 170 : 0, al, ang, sy);
}

static void draw_hud(void) {
    /* player panels */
    for (int i = 0; i < 2; i++) {
        Player *p = &pl[i]; if (!p->active && i == 1) {
            if (scene == SC_PLAY && !demo_on) draw_rich(&A.small, "P2: PRESS [+] TO JOIN", SCREEN_W - 24, 22, 0.5f, 2, 230, 210, 170, 170);
            continue; }
        float x = i == 0 ? 20 : SCREEN_W - 300, y = 16;
        fill_rect((int)x, (int)y, 280, 74, 20, 12, 8, 190); fill_rect((int)x, (int)y, 280, 3, 200, 160, 90, 255);
        draw_text_col(&A.small, i == 0 ? "P1" : "P2", x + 12, y + 8, 0.55f, 0, 255, 220, 150, 255);
        const char *gl[3] = {p->chr == 1 ? "UNDERSHIRT" : "BOXERS", p->chr == 1 ? "COVERALLS" : "OVERALLS", "FULL GEAR"};
        for (int g = 0; g < 3; g++) { int on = p->gear > g; float gx = x + 58 + g * 30;
            draw_ellipse(&A.disc, gx, y + 22, 11, 11, on ? (g == 2 ? 235 : g == 1 ? 80 : 245) : 60, on ? (g == 2 ? 185 : g == 1 ? 120 : 245) : 50, on ? (g == 2 ? 50 : g == 1 ? 190 : 245) : 46, 255); }
        draw_text_col(&A.small, gl[p->gear > 0 ? p->gear - 1 : 0], x + 150, y + 10, 0.45f, 0, 240, 230, 210, 255);
        char b[48]; snprintf(b, sizeof b, "x%d  %s", p->lives, WEAP_NAME[p->weapon]);
        draw_text_col(&A.small, b, x + 12, y + 42, 0.48f, 0, 255, 240, 200, 255);
    }
    /* building power */
    float px = SCREEN_W / 2 - 150, py = 18;
    fill_rect((int)px - 6, (int)py - 4, 312, 44, 20, 12, 8, 200);
    draw_text_col(&A.small, "POWER", px, py, 0.5f, 0, 255, 220, 150, 255);
    fill_rect((int)px + 108, (int)py + 6, 190, 16, 40, 30, 24, 255);
    Uint8 r = power < 25 ? 230 : 240, g = power < 25 ? 60 : 200;
    if (!(power < 25 && (frame_no / 15) % 2)) fill_rect((int)px + 108, (int)py + 6, (int)(power * 1.9f), 16, r, g, 70, 255);
    char b[32]; snprintf(b, sizeof b, "%d%%", (int)ceilf(power)); draw_text_col(&A.small, b, px + 203, py + 22, 0.4f, 1, 255, 240, 210, 255);
    snprintf(b, sizeof b, "SCORE %06d", score); draw_text_col(&A.small, b, SCREEN_W / 2, py + 44, 0.45f, 1, 240, 220, 180, 220);
    if (loop_no) draw_text_col(&A.small, "2ND SHOW", SCREEN_W / 2, py + 62, 0.4f, 1, 255, 90, 70, 230);
    if (rush_on) { char t2[32]; snprintf(t2, sizeof t2, "RUSH %d:%02d", (int)rush_time / 60, (int)rush_time % 60); draw_text_col(&A.small, t2, SCREEN_W / 2 + 230, py + 4, 0.5f, 1, 255, 200, 120, 255); }
    else if (scene == SC_PLAY) for (int k = 0; k < 3; k++) { int got = ((save.reels[level_no] | reels_run) >> k) & 1;
        draw_ellipse(&A.disc, SCREEN_W / 2 + 200 + k * 26, py + 16, 10, 10, got ? 255 : 60, got ? 210 : 50, got ? 120 : 40, got ? 255 : 160);
        draw_ellipse(&A.disc, SCREEN_W / 2 + 200 + k * 26, py + 16, 3, 3, 30, 20, 16, 255); }
    /* boss bar */
    if (boss.active && boss.state != BS_DEAD) {
        float bx = SCREEN_W / 2 - 300, by = SCREEN_H - 50;
        fill_rect((int)bx - 4, (int)by - 26, 608, 48, 20, 10, 8, 210);
        draw_text_col(&A.small, LEVELS[level_no].boss_name, bx, by - 24, 0.5f, 0, 255, 200, 150, 255);
        fill_rect((int)bx, (int)by + 2, 600, 14, 50, 20, 20, 255);
        fill_rect((int)bx, (int)by + 2, (int)(600.0f * boss.hp / boss.maxhp), 14, 200, 40, 40, 255);
    }
    if (boss.state == BS_INTRO && boss.active) {
        float k = clampf(boss.timer, 0, 1);
        fill_rect(0, 250, SCREEN_W, 150, 10, 6, 4, (Uint8)(200 * k));
        draw_text_col(&A.font, LEVELS[level_no].boss_name, SCREEN_W / 2, 268, 1.6f, 1, 255, 220, 150, (Uint8)(255 * k));
        draw_text_col(&A.small, LEVELS[level_no].boss_line, SCREEN_W / 2, 352, 0.7f, 1, 230, 200, 170, (Uint8)(255 * k));
    }
    if (msg_t > 0) {
        Uint8 a = (Uint8)(255 * clampf(msg_t * 2, 0, 1));
        draw_text_col(&A.font, msg, SCREEN_W / 2, 150, 0.8f, 1, 255, 236, 190, a);
    }
    if (warn_t > 0 || fiend.state == 2) {
        if ((frame_no / 8) % 2) draw_text_col(&A.font, fiend.state == 2 ? "IT'S HERE! HIDE IN A PROP CLOSET!" : "IT'S COMING... HIDE!", SCREEN_W / 2, 200, 0.85f, 1, 255, 80, 60, 255);
    }
}

static void draw_post(void) {
    /* sepia grade, grain, vignette: old film */
    SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_MOD);
    SDL_SetRenderDrawColor(R, 255, 236, 205, 255); SDL_RenderFillRect(R, NULL);
    SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_BLEND);
    Tex *g = &A.grain[(frame_no / 3) % 4];
    if (g->tex) { SDL_SetTextureBlendMode(g->tex, SDL_BLENDMODE_BLEND); draw_tex(g, 0, 0, SCREEN_W, SCREEN_H, 0, 60); }
    draw_tex(&A.vignette, 0, 0, SCREEN_W, SCREEN_H, 0, 255);
    if ((frame_no % 97) < 2) fill_rect((int)(frand() * SCREEN_W), 0, 2, SCREEN_H, 255, 240, 220, 50);   /* film scratch */
}

static void draw_dark(void) {
    if (!darkness_on) return;
    float x = 0; int n = 0;
    for (int i = 0; i < 2; i++) if (pl[i].active && pl[i].alive && !pl[i].hidden) { x += sx(pl[i].x); n++; }
    if (!n) { fill_rect(0, 0, SCREEN_W, SCREEN_H, 4, 2, 6, 245); return; }
    x /= n; float y = pl[0].y - 70, r = 330 + sinf(frame_no * 0.3f) * 6;
    draw_tex(&A.mask, x - r, y - r, r * 2, r * 2, 0, 255);
    fill_rect(0, 0, (int)(x - r) + 1, SCREEN_H, 4, 2, 6, 248); fill_rect((int)(x + r) - 1, 0, SCREEN_W, SCREEN_H, 4, 2, 6, 248);
    fill_rect((int)(x - r), 0, (int)(r * 2), (int)(y - r) + 1, 4, 2, 6, 248); fill_rect((int)(x - r), (int)(y + r) - 1, (int)(r * 2), SCREEN_H, 4, 2, 6, 248);
    /* enemy eyes glow in the dark */
    for (int i = 0; i < nen; i++) { Enemy *e = &en[i]; if (!e->alive || e->type >= E_CRATE) continue;
        float ex = sx(e->x), ey = e->y - (e->type == E_USHER ? 122 : e->type == E_DOG ? 78 : 36);
        if (ex < -50 || ex > SCREEN_W + 50) continue;
        draw_glow(&A.disc, ex - 7, ey, 12, 255, 170, 40, 255); draw_glow(&A.disc, ex + 7, ey, 12, 255, 170, 40, 255); }
}

/* ---- per-show art: a painting for each of the six zones, a foreground layer and a ground texture.
   Loaded when a show starts and freed when you leave it, so only one show's art is ever in memory. */
static const char *SHOW_DIR[NUM_LEVELS] = {"lobby", "pizza", "reels", "ink", "lost", "mirrors", "roof", "encore"};
static const float SHOW_K[NUM_LEVELS] = {1.0f, 1.0f, 1.12f, 1.12f, 1.12f, BG7_K, BG8_K, 1.03f};   /* painted floor height */
static void load_show_art(void) {
    if (zart_show == level_no) return;
    Uint32 t0 = SDL_GetTicks();
    for (int i = 0; i < 6; i++) tex_free(&zbg[i]);
    tex_free(&zfg); tex_free(&zground);
    char rel[96];
    for (int i = 0; i < 6; i++) { snprintf(rel, sizeof rel, "bg/%s/z%d.jpg", SHOW_DIR[level_no], i); zbg[i] = tex_load(rel); }
    snprintf(rel, sizeof rel, "bg/%s/fg.png", SHOW_DIR[level_no]); zfg = tex_load(rel);
    snprintf(rel, sizeof rel, "bg/%s/ground.jpg", SHOW_DIR[level_no]); zground = tex_load(rel);
    zart_show = level_no;
    SDL_Log("show %d art loaded in %u ms", level_no + 1, SDL_GetTicks() - t0);
}
static void draw_painting(Tex *t, float k, float ox, Uint8 a) {   /* one zone painting, tiled with parallax */
    float h = SCREEN_H * k, w = t->w * (h / t->h), par = camx * 0.35f, off = -fmodf(par, w);
    for (float x = off; x < SCREEN_W; x += w) draw_tex(t, x + ox * 0.3f, 0, w + 1, h, 0, a);
}
static int draw_zone_paintings(float ox) {
    float k = SHOW_K[level_no], c = camx + SCREEN_W / 2;
    int z = zone_of(c);
    if (!zbg[z].tex) return 0;
    /* crossfade across zone boundaries */
    for (int b = 1; b < 6; b++) { float d = c - ZONE_X0[b] * TILE;
        if (fabsf(d) < 360 && zbg[b - 1].tex && zbg[b].tex) {
            draw_painting(&zbg[b - 1], k, ox, 255);
            draw_painting(&zbg[b], k, ox, (Uint8)(255 * clampf((d + 360) / 720, 0, 1)));
            return 1; } }
    draw_painting(&zbg[z], k, ox, 255);
    return 1;
}
static void draw_foreground(float ox) {   /* silhouettes sliding past in front: the top band hangs down, the bottom band only peeks in */
    if (!zfg.tex) return;
    float w = zfg.w * ((float)SCREEN_H / zfg.h), off = -fmodf(camx * 1.3f, w);
    int half = zfg.h / 2;
    for (float x = off; x < SCREEN_W; x += w) {
        SDL_Rect top = {0, 0, zfg.w, half}, bot = {0, half, zfg.w, zfg.h - half};
        draw_tex_src(&zfg, &top, x + ox * 1.3f, 0, w + 1, SCREEN_H / 2.0f, 235);
        draw_tex_src(&zfg, &bot, x + ox * 1.3f, SCREEN_H / 2.0f + 104, w + 1, SCREEN_H / 2.0f, 175);   /* low enough that pits stay readable */
    }
}
static void draw_bg(float ox) {
    Tex *bgs[NUM_LEVELS] = {&A.bg, &A.bg2, &A.bg3, &A.bg4, &A.bg5, &A.bg7, &A.bg8, &A.bg6};
    Tex *bgt = bgs[level_no]->tex ? bgs[level_no] : &A.bg;
    if (draw_zone_paintings(ox)) {}
    else if (bgt->tex) {
        float k = (bgt == &A.bg3 || bgt == &A.bg4 || bgt == &A.bg5) ? 1.12f : bgt == &A.bg6 ? 1.03f : bgt == &A.bg7 ? BG7_K : bgt == &A.bg8 ? BG8_K : 1.0f;   /* the cartoon reel's painted floor sits higher: scale it down to ours */
        float par = camx * 0.35f; float w = bgt->w * k;
        float off = -fmodf(par, w);
        for (float x = off; x < SCREEN_W; x += w) draw_tex(bgt, x + ox * 0.3f, 0, w, SCREEN_H * k, 0, 255);
    } else fill_rect(0, 0, SCREEN_W, SCREEN_H, 60, 40, 30, 255);
    Uint8 dim = (Uint8)(110 - power * 0.6f);   /* the theatre dims as the power runs down */
    fill_rect(0, 0, SCREEN_W, SCREEN_H, 16, 8, 6, (Uint8)((zbg[0].tex ? 40 : 70) + dim));
    int cz = zone_of(camx + SCREEN_W / 2);
    if (lit[cz] > 0) fill_rect(0, 0, SCREEN_W, SCREEN_H, 255, 230, 170, (Uint8)(30 * fminf(1, lit[cz])));
}

static void draw_ending(void) {
    float t = scene_t;
    fill_rect(0, 0, SCREEN_W, SCREEN_H, 20, 6, 6, 255);
    for (int i = 0; i < 16; i++) {   /* the red curtains close in from both sides */
        float k = fminf(1, t / 2.5f), w = SCREEN_W * 0.5f * k, fx = i * (w / 8);
        Uint8 sh = (Uint8)(110 + 40 * sinf(i * 1.3f));
        if (i < 8) fill_rect((int)fx, 0, (int)(w / 8) + 1, SCREEN_H, sh, 18, 20, 255);
        else fill_rect((int)(SCREEN_W - w + (i - 8) * (w / 8)), 0, (int)(w / 8) + 1, SCREEN_H, sh, 18, 20, 255);
    }
    fill_rect(0, 0, SCREEN_W, 60, 90, 60, 20, 255); fill_rect(0, 56, SCREEN_W, 6, 200, 160, 60, 255);
    if (t < 2.5f) return;
    Uint8 a = (Uint8)fminf(255, (t - 2.5f) * 200);
    if (!loop_no) {
        draw_text_col(&A.font, "THE CURTAIN FALLS", SCREEN_W / 2, 120, 1.3f, 1, 255, 230, 160, a);
        draw_text_col(&A.small, "UNCLE INKY IS FINALLY AT REST...", SCREEN_W / 2, 200, 0.7f, 1, 240, 210, 170, a);
        if (t > 4.0f) draw_text_col(&A.small, "...BUT THE PROJECTOR IS STILL RUNNING.", SCREEN_W / 2, 236, 0.7f, 1, 255, 120, 90, (Uint8)fminf(255, (t - 4.0f) * 200));
    } else {
        draw_text_col(&A.font, "TRUE ENDING", SCREEN_W / 2, 120, 1.3f, 1, 255, 240, 180, a);
        draw_text_col(&A.small, "THE LIGHTS COME UP. THE INK DRAINS AWAY.", SCREEN_W / 2, 200, 0.7f, 1, 240, 220, 180, a);
        draw_text_col(&A.small, "THE NIGHT CREW CLOCKS OUT. THE SHOW IS OVER FOR GOOD.", SCREEN_W / 2, 236, 0.7f, 1, 240, 220, 180, a);
    }
    { char b[48]; snprintf(b, sizeof b, "FINAL SCORE %06d", score); draw_text_col(&A.font, b, SCREEN_W / 2, 300, 0.8f, 1, 255, 240, 210, a); }
    if (A.inky[0].tex && !loop_no && t > 5) { Tex *k = &A.inky[0]; float s = 0.35f;
        draw_tex_tint(k, SCREEN_W - k->w * s - 40, SCREEN_H - k->h * s - 30, k->w * s, k->h * s, 0, 60, 50, 50, (Uint8)(120 + 60 * sinf(t * 3)), 0); }
    if (t > 6.0f) draw_rich(&A.small, loop_no ? "PRESS [OK] TO ROLL THE CREDITS" : "PRESS [OK] - THEN THE SECOND SHOW (HARDER - THE TRUE ENDING AWAITS)", SCREEN_W / 2, 650, 0.55f, 1, 255, 240, 200, (Uint8)(150 + 100 * sinf(t * 4)));
}

static void game_draw_inner(void) {
    SDL_SetRenderDrawColor(R, 0, 0, 0, 255); SDL_RenderClear(R);
    if (draw_front()) return;
    if (scene == SC_END) { draw_ending(); draw_post(); return; }
    float ox = shake > 0 ? (frand() - 0.5f) * shake * 2 : 0, oy = shake > 0 ? (frand() - 0.5f) * shake * 2 : 0;
    if (!save.shake) ox = oy = 0;
    draw_bg(ox);
    draw_tutorial_signs(ox, oy);
    draw_boss(ox, oy);
    draw_tiles(ox, oy);
    draw_reels_world(ox, oy);
    draw_splats(ox, oy);
    if (level_no == L_ENCORE) {   /* the flooded orchestra pit */
        float x0 = sx(ZONE_X0[2] * TILE) + ox, x1 = sx(ZONE_X0[3] * TILE) + ox, surf = tide_surface() + oy;
        if (x1 > 0 && x0 < SCREEN_W && surf < FLOOR_Y + 20) {
            fill_rect((int)x0, (int)surf, (int)(x1 - x0), (int)(SCREEN_H - surf), 8, 6, 10, 235);
            for (float wx = x0; wx < x1; wx += 46) draw_ellipse(&A.disc, wx + 23, surf + sinf(frame_no * 0.08f + wx * 0.05f) * 4, 30, 9, 14, 10, 16, 255);
            for (float wx = x0; wx < x1; wx += 92) draw_ellipse(&A.disc, wx + 30, surf - 2, 8, 3, 120, 110, 130, 140);
        }
    }
    draw_storm_back(ox, oy);
    for (int i = 0; i < nen; i++) if (en[i].alive) { float x = sx(en[i].x); if (x > -200 && x < SCREEN_W + 200) draw_enemy(&en[i], ox, oy); }
    for (int i = 0; i < 2; i++) draw_player(&pl[i], ox, oy);
    draw_gibs(ox, oy);
    draw_fiend(ox, oy);
    draw_proj(ox, oy);
    draw_parts(ox, oy);
    draw_storm_front(ox, oy);
    draw_foreground(ox);
    if (negative_t > 0) {   /* the film skips: flashes of overexposed white and black frames */
        int f = (frame_no / 2) % 3;
        if (f == 0) fill_rect(0, 0, SCREEN_W, SCREEN_H, 250, 248, 240, 150); else if (f == 1) fill_rect(0, 0, SCREEN_W, SCREEN_H, 0, 0, 0, 120);
        for (int k = 0; k < 4; k++) fill_rect((int)(frand() * SCREEN_W), 0, 2 + rand() % 3, SCREEN_H, 255, 255, 255, 120);
    }
    draw_dark();
    if (warn_t > 0 || fiend.state == 2) {   /* ink creeps in from the edges */
        float k = fiend.state == 2 ? 1 : 1 - warn_t / 1.8f;
        for (int i = 0; i < 14; i++) { float yy = i * 55.0f, len = 60 + 50 * k + sinf(frame_no * 0.1f + i) * 20;
            draw_ellipse(&A.disc, 0, yy, len, 40, 8, 4, 6, 230); draw_ellipse(&A.disc, SCREEN_W, yy, len, 40, 8, 4, 6, 230); }
    }
    draw_post();
    draw_hud();
    if (scene == SC_OVER || scene == SC_CLEAR) {
        fill_rect(0, 0, SCREEN_W, SCREEN_H, 10, 4, 2, (Uint8)fminf(200, scene_t * 200));
        if (scene == SC_OVER) {
            draw_text_col(&A.font, "THE SHOW IS OVER", SCREEN_W / 2, 220, 1.4f, 1, 255, 80, 60, 255);
            draw_text_col(&A.small, rush_on ? "THE BOSSES TAKE A BOW... AT YOUR EXPENSE" : "UNCLE INKY KEEPS HIS THEATRE... FOR NOW", SCREEN_W / 2, 320, 0.7f, 1, 240, 210, 170, 255);
            if (scene_t > 2) { menu_row(410, rush_on ? "RESTART THE BOSS RUSH" : "TRY THIS SHOW AGAIN", over_sel == 0, 0, 0.6f); menu_row(472, "QUIT TO TITLE", over_sel == 1, 0, 0.6f); }
        } else if (scene_t < after_len()) draw_aftermath();
        else {
            fill_rect(0, 0, SCREEN_W, SCREEN_H, 10, 4, 2, after_len() > 0 ? 120 : 0);
            draw_clear_card();
            if (scene_t > after_len() + 2) draw_rich(&A.small, "PRESS [OK]", SCREEN_W / 2, 560, 0.6f, 1, 255, 240, 200, (Uint8)(150 + 100 * sinf(scene_t * 4)));
        }
    }
    if (paused && scene == SC_PLAY) draw_pause();
    if (scene == SC_OPTIONS) { draw_options(); draw_post(); }
}

/* ---- friendly error screens: missing game files, a controller that dropped out */
static int files_missing = -1, ctrl_lost;
static float ctrl_lost_t;
static int check_files(void) { return !A.font.tex.tex || !A.small.tex.tex || !keyart_tex.tex || !A.hero[0][0][0].tex; }
void game_controller_lost(void) { if (demo_on) return; ctrl_lost = 1; ctrl_lost_t = 0; game_focus_lost(); }
void game_controller_found(void) { ctrl_lost = 0; }
static void draw_error_card(const char *title, const char *l1, const char *l2, const char *hint) {
    fill_rect(0, 0, SCREEN_W, SCREEN_H, 8, 3, 2, 225);
    fill_rect(SCREEN_W / 2 - 420, 200, 840, 320, 40, 14, 10, 255); fill_rect(SCREEN_W / 2 - 412, 208, 824, 304, 20, 8, 6, 255);
    fill_rect(SCREEN_W / 2 - 420, 200, 840, 6, 200, 60, 40, 255);
    if (!A.font.tex.tex) { for (int i = 0; i < 6; i++) fill_rect(SCREEN_W / 2 - 14, 260 + i * 24, 28, 18, 230, 70, 50, 255); fill_rect(SCREEN_W / 2 - 14, 430, 28, 28, 230, 70, 50, 255); return; }
    draw_text_col(&A.font, title, SCREEN_W / 2, 240, fit(&A.font, title, 0.8f, 780), 1, 255, 120, 90, 255);
    draw_text_col(&A.small, l1, SCREEN_W / 2, 330, fit(&A.small, l1, 0.55f, 780), 1, 255, 236, 200, 255);
    draw_text_col(&A.small, l2, SCREEN_W / 2, 372, fit(&A.small, l2, 0.55f, 780), 1, 230, 200, 170, 255);
    draw_rich(&A.small, hint, SCREEN_W / 2, 450, 0.55f, 1, 220, 200, 170, (Uint8)(170 + 80 * sinf(SDL_GetTicks() / 250.0f)));
}
void game_step(const Input pads[MAX_PADS], const int connected[MAX_PADS]) {
    if (files_missing < 0) { const char *e = getenv("MM_ERR"); if (e && *e == '2') ctrl_lost = 1; files_missing = check_files() || (e && *e == '1'); if (files_missing) printf("Midnight Matinee: game files are missing - reinstall the .wuhb\n"); }
    unsigned anyp = 0; for (int i = 0; i < MAX_PADS; i++) anyp |= pads[i].pressed;
    if (files_missing) { if (anyp & B_START) quit_req = 1; return; }
    if (ctrl_lost) { ctrl_lost_t += DT; if (ctrl_lost_t > 0.4f && anyp) ctrl_lost = 0; return; }
    game_step_inner(pads, connected); iris_track();
}
void game_draw(void) {
    game_draw_inner();
    if (demo_on && (scene == SC_PLAY || scene == SC_LOAD)) {
        fill_rect(0, SCREEN_H - 64, SCREEN_W, 64, 8, 3, 2, 190);
        draw_text_col(&A.font, "DEMO", 70, SCREEN_H - 54, 0.6f, 1, 255, 120, 90, (Uint8)(160 + 90 * sinf(scene_t * 5)));
        draw_rich(&A.font, "PRESS [+] TO PLAY", SCREEN_W / 2, SCREEN_H - 54, 0.55f, 1, 255, 236, 190, 255);
    }
    draw_iris();
    draw_toast();
    if (files_missing > 0) draw_error_card("SOMETHING'S MISSING BACKSTAGE", "SOME OF THE GAME'S FILES COULDN'T BE FOUND.", "PLEASE REINSTALL MIDNIGHT MATINEE FROM THE .WUHB FILE.", "PRESS [+] TO QUIT");
    else if (ctrl_lost) draw_error_card("CONTROLLER DISCONNECTED", "A CONTROLLER LOST ITS CONNECTION.", "CHECK ITS BATTERY OR RECONNECT IT. THE SHOW WILL WAIT.", "PRESS ANY BUTTON TO CONTINUE");
}

/* ------------------------------------------------------------ drawing: GamePad security office */
#define PR(x, y, w, h, r, g, b, a) fill_rect((int)(x), (int)(y), (int)(w), (int)(h), r, g, b, a)

static void draw_feed(int z, float fx, float fy, float fw, float fh, int big) {
    float x0 = ZONE_X0[z] * TILE, x1 = ZONE_X0[z + 1] * TILE, zw = x1 - x0;
    float s = fminf(fw / zw, fh / (LH * TILE)); float oy = fy + (fh - LH * TILE * s) / 2, ox = fx + (fw - zw * s) / 2;
    int dark = power <= 0 && lit[z] <= 0;
    PR(fx, fy, fw, fh, 14, 16, 12, 255);
    if (!dark) {
        for (int ty = 0; ty < LH; ty++) for (int tx = ZONE_X0[z]; tx < ZONE_X0[z + 1]; tx++) {
            char c = map[ty][tx]; float x = ox + (tx * TILE - x0) * s, y = oy + ty * TILE * s, w = TILE * s + 1;
            if (c == '#') PR(x, y, w, w, 96, 102, 84, 255);
            else if (c == '=') PR(x, y, w, w * 0.25f + 1, 140, 146, 120, 255);
            else if (c == '~') PR(x, y, w, w, 4, 4, 4, 255);
            else if (c == 'C') PR(x, y, w, w, 70, 60, 44, 255);
            else if (c == 'B') PR(x + w * 0.2f, y, w * 0.6f, w, 160, 160, 140, 255);
            else if (c == 'O') PR(x, y, w, w, 150, 90, 70, 255);
            else if (c == '>' || c == '<') PR(x, y, w, w * 0.5f, 120, 120, 110, 255);
            else if (c == 'U') PR(x, y, w, w * 0.8f, 220, 150, 90, 255);
            else if (c == 'M') PR(x, y + w * 0.6f, w, w * 0.4f, 230, 200, 120, 255);
            else if (c == 'X') PR(x - w * 0.5f, oy + (crusher_bottom(tx) - 44) * s, w * 2, 44 * s, 200, 120, 80, 255);
            else if (c == 'V') PR(x, y + w * 0.6f, w, w * 0.4f, vent_on(tx) ? 240 : 140, vent_on(tx) ? 240 : 140, vent_on(tx) ? 240 : 140, 255);
            else if (c == 'F') { if (film_on(tx)) PR(x, y, w, w * 0.3f + 1, 200, 200, 190, 255); }
            else if (c == 'Y') PR(x + w * 0.25f, y, w * 0.5f, w, 170, 210, 240, 255);
            else if (c == 'n') { if (neon_on(tx)) PR(x, y, w, w * 0.3f + 1, 255, 120, 200, 255); }
            else if (c == 'S') PR(x + w * 0.15f, oy + (saw_y(tx, ty) - 24) * s, w * 0.7f, w * 0.7f, 230, 80, 60, 255);
        }
        if (lit[z] > 0) PR(fx, fy, fw, fh, 255, 240, 180, 40);
    }
    /* things on camera */
    for (int i = 0; i < nen; i++) { Enemy *e = &en[i]; if (!e->alive || zone_of(e->x) != z || e->type >= E_CRATE) continue;
        float ex = ox + (e->x - x0) * s, ey = oy + e->y * s; float w, h; enemy_box(e, &w, &h);
        if (!dark) PR(ex - w * s / 2, ey - h * s, w * s, h * s, 30, 34, 28, 255);
        float eyy = ey - h * s * 0.8f; float er = big ? 2.5f : 1.5f;
        draw_ellipse(&padDisc, ex - er * 1.6f, eyy, er, er, 255, 180, 60, 255); draw_ellipse(&padDisc, ex + er * 1.6f, eyy, er, er, 255, 180, 60, 255);
        if (e->marked > 0) { PR(ex - 6, ey - h * s - 8, 12, 3, 255, 60, 40, 255); }
    }
    for (int i = 0; i < 2; i++) { Player *p = &pl[i]; if (!p->active || !p->alive || zone_of(p->x) != z) continue;
        float px = ox + (p->x - x0) * s, py = oy + p->y * s;
        if (p->hidden) continue;
        PR(px - 20 * s, py - 120 * s, 40 * s, 120 * s, dark ? 40 : 200, dark ? 40 : 210, dark ? 40 : 190, 255);
        draw_ellipse(&padDisc, px, py - 124 * s, 26 * s, 14 * s, 230, 190, 50, 255);
    }
    if (fiend.state >= 2 && zone_of(fiend.x) == z) {
        float px = ox + (fiend.x - x0) * s, py = oy + fiend.y * s;
        draw_ellipse(&padDisc, px, py - 150 * s, 70 * s, 150 * s, 0, 0, 0, 255);
        draw_ellipse(&padDisc, px - 18 * s, py - 200 * s, 10 * s + 1, 14 * s + 1, 255, 255, 255, 255);
        draw_ellipse(&padDisc, px + 18 * s, py - 200 * s, 10 * s + 1, 14 * s + 1, 255, 255, 255, 255);
    }
    if (boss.active && z == 5 && boss.state != BS_DEAD) {
        float px = ox + (boss.x - x0) * s, py = oy + boss.y * s;
        if (!dark) draw_ellipse(&padDisc, px, py - 200 * s, 110 * s, 200 * s, 70, 60, 70, 255);
        draw_ellipse(&padDisc, px - 22 * s, py - 300 * s, 9 * s + 1, 9 * s + 1, boss.hp < boss.maxhp / 2 ? 255 : 255, boss.hp < boss.maxhp / 2 ? 40 : 190, 40, 255);
        draw_ellipse(&padDisc, px + 22 * s, py - 300 * s, 9 * s + 1, 9 * s + 1, boss.hp < boss.maxhp / 2 ? 255 : 255, boss.hp < boss.maxhp / 2 ? 40 : 190, 40, 255);
    }
    /* CCTV look: scanlines, noise, a rolling bar */
    for (float y = fy; y < fy + fh; y += 3) PR(fx, y, fw, 1, 0, 0, 0, 70);
    float roll = fy + fmodf(frame_no * 1.3f + z * 37, fh);
    PR(fx, roll, fw, 6, 200, 220, 190, 18);
    if (dark) for (int i = 0; i < (big ? 160 : 40); i++) PR(fx + frand() * fw, fy + frand() * fh, 2, 2, 200, 200, 200, (Uint8)(40 + frand() * 80));
    SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_MOD); SDL_SetRenderDrawColor(R, 190, 230, 200, 255);
    SDL_Rect rc = {(int)fx, (int)fy, (int)fw, (int)fh}; SDL_RenderFillRect(R, &rc); SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_BLEND);
}

void game_draw_pad(void) {
    PR(0, 0, PAD_W, PAD_H, 26, 18, 12, 255);
    if (paused || (scene == SC_OPTIONS && opt_back == SC_PLAY)) {
        draw_text_col(&padFont, "INTERMISSION", PAD_W / 2, 170, 1.1f, 1, 255, 220, 150, 255);
        draw_text_col(&padSmall, "THE SHOW IS PAUSED - PRESS + TO RESUME", PAD_W / 2, 250, 0.5f, 1, 230, 200, 160, 255);
        for (float y = 0; y < PAD_H; y += 3) PR(0, y, PAD_W, 1, 0, 0, 0, 50);
        return;
    }
    if (scene != SC_PLAY) {
        draw_text_col(&padFont, "SECURITY OFFICE", PAD_W / 2, 150, 1.1f, 1, 255, 220, 150, 255);
        draw_text_col(&padSmall, scene == SC_TITLE ? "CAMERAS OFFLINE UNTIL THE SHOW STARTS" : "SHIFT OVER", PAD_W / 2, 230, 0.55f, 1, 230, 200, 160, 255);
        if (scene == SC_TITLE) {
            draw_text_col(&padSmall, "SOLO: THIS GAMEPAD RUNS AND WATCHES THE CAMERAS", PAD_W / 2, 300, 0.42f, 1, 220, 190, 150, 255);
            draw_text_col(&padSmall, "CO-OP: RUNNER USES A WII REMOTE / PRO CONTROLLER", PAD_W / 2, 330, 0.42f, 1, 220, 190, 150, 255);
        }
        for (float y = 0; y < PAD_H; y += 3) PR(0, y, PAD_W, 1, 0, 0, 0, 50);
        return;
    }
    /* header */
    PR(0, 0, PAD_W, 44, 14, 8, 6, 255); PR(0, 44, PAD_W, 2, 200, 150, 80, 255);
    draw_text_col(&padSmall, "SECURITY OFFICE", 14, 10, 0.55f, 0, 255, 220, 150, 255);
    int mins = (int)(scene_t / 4) % 60, hrs = 12 + (int)(scene_t / 240); if (hrs > 12) hrs -= 12;
    char clk[32]; snprintf(clk, sizeof clk, "%d:%02d AM", hrs, mins);
    draw_text_col(&padSmall, clk, PAD_W - 14 - text_width(&padSmall, clk, 0.55f), 10, 0.55f, 0, 255, 236, 200, 255);
    if (mode == 0) draw_text_col(&padSmall, "SOLO", PAD_W / 2, 12, 0.45f, 1, 200, 180, 150, 200);
    else draw_text_col(&padSmall, "CO-OP OPERATOR", PAD_W / 2, 12, 0.45f, 1, 200, 180, 150, 200);

    /* the big feed */
    float fx = 16, fy = 58, fw = 528, fh = 330;
    PR(fx - 4, fy - 4, fw + 8, fh + 8, 70, 60, 50, 255);
    draw_feed(cam_sel, fx, fy, fw, fh, 1);
    char lab[48]; snprintf(lab, sizeof lab, "CAM %d  %s", cam_sel + 1, ZONE_NAME[cam_sel]);
    draw_text_col(&padSmall, lab, fx + 10, fy + 8, 0.5f, 0, 230, 255, 220, 255);
    if ((frame_no / 30) % 2) { draw_ellipse(&padDisc, fx + fw - 50, fy + 18, 6, 6, 255, 40, 40, 255); draw_text_col(&padSmall, "REC", fx + fw - 40, fy + 9, 0.42f, 0, 255, 120, 120, 255); }
    if (power <= 0 && lit[cam_sel] <= 0) draw_text_col(&padFont, "NO POWER", fx + fw / 2, fy + fh / 2 - 20, 0.9f, 1, 255, 80, 60, (frame_no / 20) % 2 ? 255 : 120);
    if (cam_flash > 0) PR(fx, fy, fw, fh, 255, 255, 255, (Uint8)(cam_flash * 500 > 200 ? 200 : cam_flash * 500));

    /* camera buttons */
    for (int i = 0; i < 6; i++) {
        SDL_Rect r = cam_btn(i); int sel = i == cam_sel;
        int ne = 0; for (int k = 0; k < nen; k++) if (en[k].alive && en[k].type < E_CRATE && zone_of(en[k].x) == i) ne++;
        int alert = (fiend.state >= 2 && zone_of(fiend.x) == i) || (fiend.state == 1 && i == 3) || (boss.active && i == 5 && boss.state != BS_DEAD);
        int here = 0; for (int k = 0; k < 2; k++) if (pl[k].active && zone_of(pl[k].x) == i) here = 1;
        PR(r.x, r.y, r.w, r.h, sel ? 220 : 60, sel ? 170 : 50, sel ? 90 : 40, 255);
        draw_feed(i, r.x + 3, r.y + 3, r.w - 6, r.h - 6, 0);
        if (alert && (frame_no / 10) % 2) { PR(r.x, r.y, r.w, 4, 255, 40, 30, 255); PR(r.x, r.y + r.h - 4, r.w, 4, 255, 40, 30, 255); PR(r.x, r.y, 4, r.h, 255, 40, 30, 255); PR(r.x + r.w - 4, r.y, 4, r.h, 255, 40, 30, 255); }
        char t[24]; snprintf(t, sizeof t, "CAM %d", i + 1);
        draw_text_col(&padSmall, t, r.x + 8, r.y + 6, 0.38f, 0, 230, 255, 220, 255);
        if (ne) { snprintf(t, sizeof t, "%d", ne); draw_text_col(&padSmall, t, r.x + r.w - 18, r.y + 6, 0.38f, 0, 255, 170, 60, 255); }
        if (here) draw_ellipse(&padDisc, r.x + r.w - 12, r.y + r.h - 12, 6, 6, 240, 200, 60, 255);
        if (lit[i] > 0) draw_ellipse(&padDisc, r.x + 12, r.y + r.h - 12, 6, 6, 255, 250, 210, 255);
    }
    /* actions */
    int can_ping = ping_cool <= 0 && power >= 6, can_light = power >= 10 && lit[cam_sel] <= 0;
    PR(BTN_PING.x, BTN_PING.y, BTN_PING.w, BTN_PING.h, can_ping ? 150 : 60, can_ping ? 40 : 40, 34, 255);
    PR(BTN_PING.x + 4, BTN_PING.y + 4, BTN_PING.w - 8, BTN_PING.h - 8, can_ping ? 200 : 70, can_ping ? 56 : 50, 44, 255);
    draw_text_col(&padFont, "PING", BTN_PING.x + BTN_PING.w / 2, BTN_PING.y + 6, 0.7f, 1, 255, 240, 220, 255);
    draw_text_col(&padSmall, "(-)  6%", BTN_PING.x + BTN_PING.w / 2, BTN_PING.y + 40, 0.36f, 1, 255, 220, 200, 255);
    PR(BTN_LIGHTS.x, BTN_LIGHTS.y, BTN_LIGHTS.w, BTN_LIGHTS.h, can_light ? 170 : 60, can_light ? 140 : 50, 40, 255);
    PR(BTN_LIGHTS.x + 4, BTN_LIGHTS.y + 4, BTN_LIGHTS.w - 8, BTN_LIGHTS.h - 8, can_light ? 230 : 70, can_light ? 190 : 60, 60, 255);
    draw_text_col(&padFont, "LIGHTS", BTN_LIGHTS.x + BTN_LIGHTS.w / 2, BTN_LIGHTS.y + 6, 0.7f, 1, 40, 24, 10, 255);
    draw_text_col(&padSmall, "(X)  10%", BTN_LIGHTS.x + BTN_LIGHTS.w / 2, BTN_LIGHTS.y + 40, 0.36f, 1, 60, 36, 16, 255);
    /* power gauge */
    float gx = 382, gy = 408, gw = 162;
    draw_text_col(&padSmall, "POWER", gx, gy - 2, 0.42f, 0, 255, 220, 150, 255);
    char pw[16]; snprintf(pw, sizeof pw, "%d%%", (int)ceilf(power)); draw_text_col(&padSmall, pw, gx + gw - text_width(&padSmall, pw, 0.42f), gy - 2, 0.42f, 0, 255, 240, 200, 255);
    PR(gx, gy + 26, gw, 28, 40, 30, 24, 255);
    for (int i = 0; i < 10; i++) if (power > i * 10) PR(gx + 3 + i * 16, gy + 29, 13, 22, power < 25 ? 230 : 120 + i * 12, power < 25 ? 60 : 200, 70, 255);
    draw_text_col(&padSmall, "ZL / ZR OR TAP: SWITCH CAMERA", 560 + 138, 312, 0.34f, 1, 210, 190, 160, 255);
    draw_text_col(&padSmall, "LIGHTS SCARE THE INK FIEND", 560 + 138, 336, 0.34f, 1, 210, 190, 160, 255);
    draw_text_col(&padSmall, "PING = DOUBLE DAMAGE", 560 + 138, 360, 0.34f, 1, 210, 190, 160, 255);
    if ((fiend.state == 1 || fiend.state == 2) && (frame_no / 10) % 2) {
        PR(560, 390, 280, 76, 120, 10, 10, 255);
        draw_text_col(&padFont, "FIEND ALERT", 700, 400, 0.6f, 1, 255, 230, 220, 255);
        draw_text_col(&padSmall, "CAM 4: HIT LIGHTS!", 700, 436, 0.38f, 1, 255, 220, 200, 255);
    }
    for (float y = 0; y < PAD_H; y += 4) PR(0, y, PAD_W, 1, 0, 0, 0, 30);
}

/* ---- test watchdog (MM_CHECK): reports impossible states during scripted runs ---- */
void game_set_checkpoint(int c) { if (c >= 0 && c < ncheck) { cur_check = c; reset_player(&pl[0], 0); camx = clampf(pl[0].x - SCREEN_W * 0.35f, 0, LW * TILE - SCREEN_W); } }   /* tests */
void game_check(long frame) {
    static int stuck[2], fallen[2], reported = 0;
    if (reported > 40) return;
    #define REPORT(...) do { printf("CHECK f%ld scene%d show%d loop%d: ", frame, scene, level_no + 1, loop_no); printf(__VA_ARGS__); printf("\n"); reported++; } while (0)
    if (scene != SC_PLAY) { stuck[0] = stuck[1] = fallen[0] = fallen[1] = 0; return; }
    for (int i = 0; i < 2; i++) {
        Player *p = &pl[i]; if (!p->active) continue;
        if (!isfinite(p->x) || !isfinite(p->y) || !isfinite(p->vx) || !isfinite(p->vy)) REPORT("P%d non-finite pos/vel", i + 1);
        if (!p->alive) { stuck[i] = fallen[i] = 0; continue; }
        if (p->x < 0 || p->x > LW * TILE) REPORT("P%d outside level x=%.0f", i + 1, p->x);
        if (p->y > LH * TILE + 96) { if (++fallen[i] == 90) REPORT("P%d fell below the world for 1.5s without dying y=%.0f x=%.0f", i + 1, p->y, p->x); } else fallen[i] = 0;
        int tx = (int)(p->x / TILE), ty = (int)((p->y - 30) / TILE);
        if (solid_at(tx, ty) && ty >= 0) { if (++stuck[i] == 60) REPORT("P%d inside a wall for 1s at tile %d,%d (%c)", i + 1, tx, ty, map[ty][tx]); } else stuck[i] = 0;
        if (p->lives < 0 || p->lives > 99) REPORT("P%d lives=%d", i + 1, p->lives);
    }
    int alive = 0;
    for (int i = 0; i < nen; i++) { Enemy *e = &en[i]; if (!e->alive) continue; alive++;
        if (!isfinite(e->x) || !isfinite(e->y)) REPORT("enemy %d type %d non-finite", i, e->type);
        else if (e->y > LH * TILE + 600) REPORT("enemy %d type %d fell out of the world y=%.0f (never cleaned up)", i, e->type, e->y); }
    if (nen >= MAXE) { static long last; if (frame - last > 600) { REPORT("enemy table full (%d slots, %d alive)", nen, alive); last = frame; } }
    if (!isfinite(boss.hp + 0.0f)) REPORT("boss hp non-finite");
    #undef REPORT
}


/* ==================================================================================================
   THE FRONT OF HOUSE: saving, menus, pause, the loading card, grades, extras, boss rush, intro, credits
   ================================================================================================== */
static Tex intro_tex[4], gal_tex;
static int opt_sel, opt_back, sel_show, extras_sel, gal_sel, gal_view, juke_sel, juke_playing = -1, ward_sel, erase_armed;
static int intro_panel, intro_then, gal_loaded = -1, show_score0, rush_new;
static float rumble_s[MAX_PADS]; static int rumble_ms[MAX_PADS];
static struct { int level, keep, pending; Player crew[2]; int score; } ld;

static Tex *boss_tex(int s) {
    Tex *t[NUM_LEVELS] = {&A.inky[0], &A.rat[0], &A.splicer[0], &A.press[0], &A.collector[0], &A.vanity[0], &A.headliner[0], &A.unbound[0]};
    return t[s];
}
static int popcount3(int b) { return (b & 1) + ((b >> 1) & 1) + ((b >> 2) & 1); }
static int difficulty(void) { return rush_on ? 1 : save.difficulty; }
static const char *DIFF_NAME[3] = {"MATINEE (EASY)", "NORMAL", "MIDNIGHT (HARD)"};
static const char *GRADE_NAME[5] = {"-", "C", "B", "A", "S"};
static const struct { const char *name; Uint8 r, g, b; int need; } OUTFIT[5] = {   /* colours match OUTFIT_RGB */
    {"CLASSIC", 255, 255, 255, 0}, {"NIGHT SHIFT", 150, 172, 255, 6}, {"INK STAINED", 128, 116, 140, 12},
    {"GOLDEN TICKET", 255, 214, 120, 18}, {"SILVER SCREEN", 206, 214, 232, 24}};

static void rumble_pad(int pad, float s, int ms) { if (pad < 0 || pad >= MAX_PADS) return; if (s > rumble_s[pad]) rumble_s[pad] = s; if (ms > rumble_ms[pad]) rumble_ms[pad] = ms; }
static void rumble_all(float s, int ms) { for (int i = 0; i < 2; i++) if (pl[i].active) rumble_pad(pl[i].pad, s, ms); }
int game_take_rumble(int pad, float *strength, int *ms) {
    if (pad < 0 || pad >= MAX_PADS || rumble_ms[pad] <= 0) return 0;
    *strength = rumble_s[pad]; *ms = rumble_ms[pad]; rumble_s[pad] = 0; rumble_ms[pad] = 0;
    return save.rumble != 0;
}
int game_layout(void) { return save.layout; }
void game_focus_lost(void) { if (scene == SC_PLAY && !paused) { paused = 1; pause_sel = 0; audio_duck(1); } }

/* buttons by what they mean on the controller (the layout option swaps A and B) */
static unsigned OK_BITS(void) { return B_START | (save.layout ? B_JUMP : B_SHOOT); }
static unsigned BACK_BITS(void) { return B_PING | (save.layout ? B_SHOOT : B_JUMP); }

static int has_progress(void) { return save.reached[0] > 0 || save.cleared[0][0] || save.seen_ending; }
static int show_unlocked(int s) { return s == 0 || s <= save.reached[0] || s <= save.reached[1] || save.cleared[0][s] || save.cleared[1][s]; }
static int rush_unlocked(void) { return save.cleared[0][NUM_LEVELS - 1] || save.cleared[1][NUM_LEVELS - 1] || getenv("MM_UNLOCK"); }

static void go_title(void) {
    paused = 0; audio_duck(0); rush_on = 0; loop_no = 0; scene = SC_TITLE; scene_t = 0; menu_sel = 1;
    music_play("mm_title", 1); game_save();
}
static int crew_ready(void) {   /* security co-op needs a second controller for the runner */
    mode = save.style;
    if (mode == 1 && !con_now[1]) { sfx("tick"); message("CONNECT A WII REMOTE OR PRO CONTROLLER FOR THE RUNNER", 2.5f); return 0; }
    return 1;
}
/* every show starts behind the "NOW SHOWING" card: it covers the 2-3 s the Wii U spends reading the show's art */
static void queue_show(int level, int keep) {
    ld.level = level; ld.keep = keep; ld.pending = 1; memcpy(ld.crew, pl, sizeof pl); ld.score = score;
    paused = 0; audio_duck(0); scene = SC_LOAD; scene_t = 0; music_stop();
    tex_free(&aftermath_tex);
    if (!rush_on && !demo_on) {
        if (level > save.reached[loop_no]) save.reached[loop_no] = (unsigned char)level;
        save.cont_level = (unsigned char)level; save.cont_loop = (unsigned char)loop_no; save.runner = (unsigned char)(p1_char < 0 ? 0 : p1_char);
        game_save();
    }
}
static void finish_load(void) {
    level_no = ld.level; start_level();
    if (ld.keep) { score = ld.score;
        for (int i = 0; i < 2; i++) if (ld.crew[i].active) { pl[i].active = 1; pl[i].pad = ld.crew[i].pad; pl[i].lives = ld.crew[i].lives + (ld.keep == 2 ? 2 : 0);
            pl[i].weapon = ld.crew[i].weapon; reset_player(&pl[i], 0); } }
    if (rush_on) {   /* straight to the boss door */
        cur_check = ncheck - 1;
        for (int i = 0; i < 2; i++) if (pl[i].active) { int lives = pl[i].lives, w = pl[i].weapon; reset_player(&pl[i], 0); pl[i].lives = lives; pl[i].weapon = w; }
        camx = clampf(pl[0].x - SCREEN_W * 0.35f, 0, LW * TILE - SCREEN_W);
    }
    if (demo_on) { cur_check = ncheck > 3 ? 1 + rand() % 2 : 0; reset_player(&pl[0], 0); camx = clampf(pl[0].x - SCREEN_W * 0.35f, 0, LW * TILE - SCREEN_W); }
    show_time = 0; show_deaths = 0; reels_run = 0; show_score0 = score; ld.pending = 0; show_hit = boss_hit = 0;
}
static void show_cleared(void) {
    scene = SC_CLEAR; scene_t = 0; music_play("mm_clear", 0); show_newbest = 0;
    if (demo_on) return;
    vo(pl[0].active ? pl[0].chr : pl[1].chr, "win");
    load_aftermath(level_no);
    if (rush_on) return;
    int L = level_no, sc = score - show_score0;
    int pts = 100 - show_deaths * 15 - (int)fmaxf(0, (show_time - 360) / 5) + popcount3(reels_run) * 5;   /* par: six minutes */
    show_grade = pts >= 95 ? 4 : pts >= 80 ? 3 : pts >= 60 ? 2 : 1;
    if (show_grade > save.grade[L]) { save.grade[L] = (unsigned char)show_grade; show_newbest |= 1; }
    if (sc > save.best_score[L]) { save.best_score[L] = sc; show_newbest |= 2; }
    int tt = (int)(show_time * 10); if (!save.best_time[L] || tt < save.best_time[L]) { save.best_time[L] = tt; show_newbest |= 4; }
    save.cleared[loop_no][L] = 1;
    if (L == 0) award(0);
    if (L == 3) award(1);
    if (L == NUM_LEVELS - 1) award(2);
    if (!show_hit) award(9);
    if (!boss_hit) award(10);
    if (show_time < 240) award(11);
    if (difficulty() == 2) award(13);
    if (pl[0].active && pl[1].active) award(16);
    if (mode == 1) award(17);
    if (show_grade == 4) award(7);
    { int all = 1; for (int i = 0; i < NUM_LEVELS; i++) if (save.grade[i] < 4) all = 0; if (all) award(8); }
    if (L + 1 < NUM_LEVELS) { if (L + 1 > save.reached[loop_no]) save.reached[loop_no] = (unsigned char)(L + 1); save.cont_level = (unsigned char)(L + 1); save.cont_loop = (unsigned char)loop_no; }
    game_save();
}
static void start_intro(int then) {
    char rel[64]; for (int i = 0; i < 4; i++) { tex_free(&intro_tex[i]); snprintf(rel, sizeof rel, "ui/intro_%d.jpg", i); intro_tex[i] = tex_load(rel); }
    intro_panel = 0; intro_then = then; scene = SC_INTRO; scene_t = 0; music_play("mm_title", 1);
}
static void end_intro(void) {
    for (int i = 0; i < 4; i++) tex_free(&intro_tex[i]);
    save.seen_intro = 1; game_save();
    if (intro_then) { loop_no = 0; rush_on = 0; queue_show(0, 0); } else { scene = SC_EXTRAS; scene_t = 0; music_play("mm_title", 1); }
}
static void start_credits(int then) { credits_then = then; scene = SC_CREDITS; scene_t = 0; music_play("mm_ending", 1); }

/* ---- the jukebox ---- */
typedef struct { char name[24], label[64]; int show, kind; } Track;   /* kind 0 always, 1 show reached, 2 boss beaten, 3 any show beaten, 4 ending seen */
static Track tracks[40]; static int ntracks;
static void build_tracks(void) {
    ntracks = 0;
#define ADD(n, l, s, k) do { snprintf(tracks[ntracks].name, 24, "%s", n); snprintf(tracks[ntracks].label, 64, "%s", l); tracks[ntracks].show = s; tracks[ntracks].kind = k; ntracks++; } while (0)
    ADD("mm_title", "THE MARQUEE  (TITLE)", 0, 0);
    for (int s = 0; s < NUM_LEVELS; s++) {
        char l[64]; snprintf(l, 64, "SHOW %d: %s", s + 1, LEVELS[s].title); ADD(LEVELS[s].music[0], l, s, 1);
        if (strcmp(LEVELS[s].music[3], LEVELS[s].music[0])) { snprintf(l, 64, "SHOW %d: %s", s + 1, LEVELS[s].zone[3]); ADD(LEVELS[s].music[3], l, s, 1); }
        snprintf(l, 64, "SHOW %d BOSS: %s", s + 1, LEVELS[s].boss_name); ADD(LEVELS[s].boss_music, l, s, 2);
    }
    ADD("mm_clear", "SHOW CLEARED!", 0, 3); ADD("mm_gameover", "THE SHOW IS OVER", 0, 0); ADD("mm_ending", "THE CURTAIN FALLS", 0, 4);
#undef ADD
}
static int track_unlocked(int i) {
    Track *t = &tracks[i];
    switch (t->kind) { case 1: return show_unlocked(t->show); case 2: return save.cleared[0][t->show] || save.cleared[1][t->show];
    case 3: for (int s = 0; s < NUM_LEVELS; s++) if (save.cleared[0][s]) return 1; return 0; case 4: return save.seen_ending; default: return 1; }
}
/* ---- the gallery: 3 pieces per show, one per film reel ---- */
static int gal_unlocked(int i) { return (save.reels[i / 3] >> (i % 3)) & 1; }
static void gal_load(int i) {
    if (gal_loaded == i) return; tex_free(&gal_tex); gal_loaded = i;
    if (i % 3 == 0) return;   /* the boss portrait uses the sprite already in memory */
    char rel[96]; snprintf(rel, sizeof rel, "bg/%s/z%d.jpg", SHOW_DIR[i / 3], i % 3 == 1 ? 1 : 4); gal_tex = tex_load(rel);
}
static const char *gal_caption(int i, char *b, int n) {
    int s = i / 3;
    if (i % 3 == 0) snprintf(b, n, "SHOW %d - %s: %s", s + 1, LEVELS[s].boss_name, LEVELS[s].boss_line);
    else snprintf(b, n, "SHOW %d - %s", s + 1, LEVELS[s].zone[i % 3 == 1 ? 1 : 4]);
    return b;
}

/* ------------------------------------------------------------ menu steps */
static void step_pause(unsigned anyp) {
    int n = 4;
    if (anyp & B_UP) { pause_sel = (pause_sel + n - 1) % n; sfx("menu"); }
    if (anyp & B_DOWN) { pause_sel = (pause_sel + 1) % n; sfx("menu"); }
    if (anyp & BACK_BITS()) { paused = 0; audio_duck(0); sfx("menu"); return; }
    if (anyp & OK_BITS()) { sfx("menu");
        if (pause_sel == 0 || (anyp & B_START)) { paused = 0; audio_duck(0); }
        else if (pause_sel == 1) { if (rush_on) { rush_time = 0; queue_show(0, 0); } else queue_show(level_no, 0); }
        else if (pause_sel == 2) { scene = SC_OPTIONS; opt_back = SC_PLAY; opt_sel = 0; erase_armed = 0; }
        else go_title();
    }
}
static void step_title(unsigned anyp) {
    enum { T_RUNNER, T_PLAY, T_SELECT, T_STYLE, T_DIFF, T_RUSH, T_EXTRAS, T_OPTIONS, T_N };
    if (anyp & BACK_BITS() & ~B_JUMP) { sfx("menu"); open_slots(); return; }   /* - : back to the save slots */
    if (anyp & B_UP) { menu_sel = (menu_sel + T_N - 1) % T_N; sfx("menu"); }
    if (anyp & B_DOWN) { menu_sel = (menu_sel + 1) % T_N; sfx("menu"); }
    int lr = (anyp & B_LEFT) ? -1 : (anyp & B_RIGHT) ? 1 : 0, ok = (anyp & OK_BITS()) != 0;
    switch (menu_sel) {
    case T_RUNNER: if (lr || ok) { p1_char = (p1_char + 1) % NCHARS; save.runner = (unsigned char)p1_char; sfx("menu"); } break;
    case T_STYLE: if (lr || ok) { save.style ^= 1; sfx("menu"); } break;
    case T_DIFF: if (lr || ok) { save.difficulty = (unsigned char)((save.difficulty + 3 + (lr ? lr : 1)) % 3); sfx("menu"); } break;
    case T_PLAY: if (ok && crew_ready()) { sfx("checkpoint"); rush_on = 0;
            if (!has_progress() && !save.seen_intro) start_intro(1);
            else if (!has_progress()) { loop_no = 0; queue_show(0, 0); }
            else { loop_no = save.cont_loop; queue_show(save.cont_level, 0); } } break;
    case T_SELECT: if (ok) { sfx("menu"); scene = SC_SELECT; scene_t = 0; sel_show = save.cont_level; } break;
    case T_RUSH: if (ok) { if (!rush_unlocked()) { sfx("tick"); message("BEAT SHOW 8 TO OPEN THE BOSS RUSH", 2.2f); }
            else if (crew_ready()) { sfx("checkpoint"); rush_on = 1; rush_time = 0; loop_no = 0; queue_show(0, 0); } } break;
    case T_EXTRAS: if (ok) { sfx("menu"); scene = SC_EXTRAS; scene_t = 0; extras_sel = 0; } break;
    case T_OPTIONS: if (ok) { sfx("menu"); scene = SC_OPTIONS; scene_t = 0; opt_back = SC_TITLE; opt_sel = 0; erase_armed = 0; } break;
    }
}
static void step_select(unsigned anyp) {
    if (anyp & B_LEFT) { sel_show = (sel_show + NUM_LEVELS - 1) % NUM_LEVELS; sfx("menu"); }
    if (anyp & B_RIGHT) { sel_show = (sel_show + 1) % NUM_LEVELS; sfx("menu"); }
    if (anyp & (B_UP | B_DOWN)) { sel_show = (sel_show + 4) % NUM_LEVELS; sfx("menu"); }
    if (anyp & BACK_BITS()) { sfx("menu"); scene = SC_TITLE; scene_t = 0; return; }
    if (anyp & OK_BITS()) {
        if (!show_unlocked(sel_show)) { sfx("tick"); message("REACH THIS SHOW TO UNLOCK IT", 1.8f); return; }
        if (!crew_ready()) return;
        sfx("checkpoint"); rush_on = 0; loop_no = 0; queue_show(sel_show, 0);
    }
}
static void step_options(unsigned anyp) {
    int n = opt_back == SC_PLAY ? 7 : 8;   /* no erasing from the pause menu */
    if (anyp & B_UP) { opt_sel = (opt_sel + n - 1) % n; sfx("menu"); erase_armed = 0; }
    if (anyp & B_DOWN) { opt_sel = (opt_sel + 1) % n; sfx("menu"); erase_armed = 0; }
    int lr = (anyp & B_LEFT) ? -1 : (anyp & B_RIGHT) ? 1 : 0, ok = (anyp & OK_BITS()) != 0;
    int row = opt_sel < 5 ? opt_sel : opt_sel == 5 ? 7 : (n == 7 ? 8 : opt_sel == 6 ? 5 : 8);   /* 7 = language, 5 = erase, 8 = back */
    if (anyp & BACK_BITS()) row = 8, ok = 1, lr = 0;
    switch (row) {
    case 0: if (lr) { save.music_vol = (unsigned char)clampf(save.music_vol + lr, 0, 10); audio_set_volume(save.music_vol, save.sfx_vol); } break;
    case 1: if (lr) { save.sfx_vol = (unsigned char)clampf(save.sfx_vol + lr, 0, 10); audio_set_volume(save.music_vol, save.sfx_vol); sfx("coin"); } break;
    case 2: if (lr || ok) { save.rumble ^= 1; sfx("menu"); if (save.rumble) rumble_pad(0, 0.6f, 250); } break;
    case 3: if (lr || ok) { save.layout ^= 1; sfx("menu"); } break;
    case 4: if (lr || ok) { save.shake ^= 1; sfx("menu"); if (save.shake) shake = 10; } break;
    case 5: if (ok) { if (!erase_armed) { erase_armed = 1; sfx("tick"); }
                      else { save_reset_progress(); game_save(); erase_armed = 0; sfx("boom"); message("SAVE DATA ERASED. A FRESH SHOW AWAITS.", 2.2f); } } break;
    case 7: if (lr || ok) { save.language ^= 1; sfx("menu"); } break;
    case 8: if (ok) { game_save(); sfx("menu"); scene = opt_back; scene_t = opt_back == SC_PLAY ? scene_t : 0; } break;
    }
    if (lr && row < 2) sfx("menu");
}
static void step_extras(unsigned anyp) {
    const int n = 7;
    if (anyp & B_UP) { extras_sel = (extras_sel + n - 1) % n; sfx("menu"); }
    if (anyp & B_DOWN) { extras_sel = (extras_sel + 1) % n; sfx("menu"); }
    if (anyp & BACK_BITS()) { sfx("menu"); scene = SC_TITLE; scene_t = 0; return; }
    if (!(anyp & OK_BITS())) return;
    sfx("menu");
    switch (extras_sel) {
    case 0: scene = SC_GALLERY; gal_view = 0; break;
    case 1: scene = SC_JUKEBOX; if (!ntracks) build_tracks(); break;
    case 2: scene = SC_WARDROBE; ward_sel = save.outfit; break;
    case 3: scene = SC_BADGES; break;
    case 4: start_intro(0); break;
    case 5: start_credits(0); break;
    default: scene = SC_TITLE; break;
    }
    scene_t = 0;
}
static void step_gallery(unsigned anyp) {
    if (gal_view) {
        int d = (anyp & B_LEFT) ? -1 : (anyp & B_RIGHT) ? 1 : 0;
        if (d) for (int k = 1; k <= 24; k++) { int j = (gal_sel + d * k + 48) % 24; if (gal_unlocked(j)) { gal_sel = j; gal_load(j); sfx("menu"); break; } }
        if (anyp & (BACK_BITS() | OK_BITS())) { gal_view = 0; tex_free(&gal_tex); gal_loaded = -1; sfx("menu"); }
        return;
    }
    if (anyp & B_LEFT) { gal_sel = (gal_sel + 21) % 24; sfx("menu"); }
    if (anyp & B_RIGHT) { gal_sel = (gal_sel + 3) % 24; sfx("menu"); }
    if (anyp & B_UP) { gal_sel = gal_sel / 3 * 3 + (gal_sel % 3 + 2) % 3; sfx("menu"); }
    if (anyp & B_DOWN) { gal_sel = gal_sel / 3 * 3 + (gal_sel % 3 + 1) % 3; sfx("menu"); }
    if (anyp & BACK_BITS()) { sfx("menu"); scene = SC_EXTRAS; return; }
    if (anyp & OK_BITS()) { if (gal_unlocked(gal_sel)) { gal_view = 1; gal_load(gal_sel); sfx("menu"); } else { sfx("tick"); message("FIND THIS SHOW'S FILM REEL TO DEVELOP IT", 1.8f); } }
}
static void step_jukebox(unsigned anyp) {
    if (anyp & B_UP) { juke_sel = (juke_sel + ntracks - 1) % ntracks; sfx("menu"); }
    if (anyp & B_DOWN) { juke_sel = (juke_sel + 1) % ntracks; sfx("menu"); }
    if (anyp & BACK_BITS()) { sfx("menu"); scene = SC_EXTRAS; juke_playing = -1; music_play("mm_title", 1); return; }
    if (anyp & OK_BITS()) {
        if (!track_unlocked(juke_sel)) { sfx("tick"); message("KEEP PLAYING TO UNLOCK THIS TRACK", 1.6f); return; }
        juke_playing = juke_sel; music_stop(); music_play(tracks[juke_sel].name, 1);
    }
}
static void step_wardrobe(unsigned anyp) {
    if (anyp & (B_UP | B_LEFT)) { ward_sel = (ward_sel + 4) % 5; sfx("menu"); }
    if (anyp & (B_DOWN | B_RIGHT)) { ward_sel = (ward_sel + 1) % 5; sfx("menu"); }
    if (anyp & BACK_BITS()) { sfx("menu"); game_save(); scene = SC_EXTRAS; return; }
    if (anyp & OK_BITS()) {
        if (save_total_reels() < OUTFIT[ward_sel].need) { sfx("tick"); message("FIND MORE FILM REELS TO UNLOCK THIS OUTFIT", 1.8f); return; }
        save.outfit = (unsigned char)ward_sel; sfx("pickup"); if (ward_sel) award(18); game_save();
    }
}
static const char *INTRO_TEXT[4] = {
    "MIDNIGHT. THE GRAND MATINEE THEATRE HAS BEEN DARK FOR FIFTY YEARS...",
    "JOE AND ROSIE TOOK THE NIGHT SHIFT: GET THE OLD PLACE'S POWER BACK ON.",
    "BUT THE MOMENT THE LIGHTS CAME UP, THE OLD PROJECTOR STARTED ROLLING ALL BY ITSELF.",
    "UNCLE INKY IS BACK, AND HIS SHOW NEVER ENDS. KEEP THE LIGHTS ON. SURVIVE THE MATINEE."};
#define INTRO_PANEL_T 7.0f
static void step_intro(unsigned anyp) {
    if (anyp & B_START) { end_intro(); return; }
    float typed = scene_t * 34;   /* characters shown so far */
    size_t len = strlen(TR(INTRO_TEXT[intro_panel]));
    if (anyp & OK_BITS() & ~B_START) {
        if (typed < (float)len) scene_t = len / 34.0f + 0.01f;
        else { scene_t = INTRO_PANEL_T; }
    }
    if (scene_t >= INTRO_PANEL_T) { if (++intro_panel >= 4) { end_intro(); return; } scene_t = 0; }
}
static const char *CREDITS[] = {
    "#MIDNIGHT MATINEE", "", "A GAME BY", "@EFRAIN GOMEZ", "", "", "STARRING THE NIGHT CREW", "@JOE", "@ROSIE", "", "",
    "AND TONIGHT'S SHOWS", "*0", "*1", "*2", "*3", "*4", "*5", "*6", "*7", "", "",
    "ART, ANIMATION, MUSIC AND SOUND", "@MADE WITH RUNWAY", "", "PROGRAMMING", "@BUILT WITH CLAUDE", "", "ENGINE", "@C + SDL2 ON THE WII U, VIA WUT AND AROMA", "", "",
    "NO ANIMATRONICS WERE HARMED", "(SEVERAL WERE DEEPLY INCONVENIENCED)", "", "", "", "#THANKS FOR WATCHING THE SHOW", "", "SEE YOU AT THE NEXT MATINEE..."};
#define NCRED (int)(sizeof CREDITS / sizeof *CREDITS)
static float credits_y(void) { return SCREEN_H + 20 - scene_t * 70; }
static void step_credits(unsigned anyp, unsigned held) {
    if (held & OK_BITS()) scene_t += DT * 3;   /* hold A to fast-forward */
    int done = credits_y() + NCRED * 52 + 200 < 0;
    if ((anyp & B_START) || done) {
        if (credits_then == 1) { loop_no = 1; queue_show(0, 2); }   /* on to the second show */
        else if (credits_then == 2) go_title();
        else { scene = SC_EXTRAS; scene_t = 0; music_play("mm_title", 1); }
    }
}

/* ------------------------------------------------------------ menu drawing */
static void draw_curtains(float open) {   /* red stage curtains framing a menu screen */
    fill_rect(0, 0, SCREEN_W, SCREEN_H, 18, 6, 6, 255);
    for (int i = 0; i < 20; i++) { Uint8 sh = (Uint8)(70 + 30 * sinf(i * 1.7f)); float w = 30, x = i * w * (1 - open);
        fill_rect((int)x, 0, (int)w + 1, SCREEN_H, sh, 12, 14, 255); fill_rect((int)(SCREEN_W - x - w), 0, (int)w + 1, SCREEN_H, sh, 12, 14, 255); }
    fill_rect(0, 0, SCREEN_W, 46, 80, 50, 16, 255); fill_rect(0, 42, SCREEN_W, 5, 210, 170, 70, 255);
}
static void draw_header(const char *s) { draw_text_col(&A.font, s, SCREEN_W / 2, 70, 0.95f, 1, 255, 226, 160, 255); }
static void draw_hint(const char *s) { draw_rich(&A.small, s, SCREEN_W / 2, 656, 0.55f, 1, 220, 200, 170, (Uint8)(170 + 80 * sinf(scene_t * 4))); }
static void draw_msg(void) { if (msg_t > 0) draw_text_col(&A.small, msg, SCREEN_W / 2, 692, 0.55f, 1, 255, 120, 90, 255); }
static void draw_reel_icon(float x, float y, float r, int got, Uint8 a) {
    Uint8 c = got ? 255 : 70;
    if (got) draw_glow(&A.disc, x, y, r * 2.2f, 255, 200, 90, (Uint8)(a * 0.6f));
    draw_ellipse(&A.disc, x, y, r, r, got ? 60 : 40, got ? 52 : 36, got ? 50 : 34, a);
    draw_ellipse(&A.disc, x, y, r * 0.86f, r * 0.86f, c, (Uint8)(c * 0.82f), (Uint8)(c * 0.5f), a);
    for (int k = 0; k < 5; k++) { float an = k * 1.2566f + scene_t * (got ? 1.5f : 0); draw_ellipse(&A.disc, x + cosf(an) * r * 0.5f, y + sinf(an) * r * 0.5f, r * 0.2f, r * 0.2f, 30, 20, 16, a); }
    draw_ellipse(&A.disc, x, y, r * 0.14f, r * 0.14f, 30, 20, 16, a);
}
static void draw_reel_row(float cx, float y, int bits, float r) { for (int k = 0; k < 3; k++) draw_reel_icon(cx + (k - 1) * r * 2.6f, y, r, (bits >> k) & 1, 255); }
static void fmt_time(char *b, int n, int tenths) { if (!tenths) { snprintf(b, n, "--:--"); return; } snprintf(b, n, "%d:%02d.%d", tenths / 600, (tenths / 10) % 60, tenths % 10); }
static void draw_grade(float x, float y, int g, float scale, Uint8 a) {
    static const Uint8 col[5][3] = {{90, 80, 70}, {150, 150, 150}, {90, 150, 255}, {255, 90, 70}, {255, 214, 80}};
    draw_ellipse(&A.disc, x, y, 46 * scale, 46 * scale, 30, 14, 10, a);
    draw_ellipse(&A.disc, x, y, 40 * scale, 40 * scale, col[g][0], col[g][1], col[g][2], a);
    draw_ellipse(&A.disc, x, y, 33 * scale, 33 * scale, 30, 14, 10, a);
    draw_text_col(&A.font, GRADE_NAME[g], x, y - 30 * scale, 1.1f * scale, 1, col[g][0], col[g][1], col[g][2], a);
}
static void draw_sprite_tint(Tex *t, float x, float feet, float scale, int face_right, Uint8 alpha, Uint8 r, Uint8 g, Uint8 b) {
    if (!t->tex) return; float w = t->w * scale, h = t->h * scale;
    draw_tex_tint(t, x - w / 2, feet - h, w, h, !face_right, r, g, b, alpha, 0);
}
static void menu_row(float y, const char *s, int sel, int dim, float scale) {
    if (sel) { fill_rect(SCREEN_W / 2 - 360, (int)y - 6, 720, (int)(64 * scale + 6), 120, 26, 22, 210);
        draw_ellipse(&A.disc, SCREEN_W / 2 - 340, y + 32 * scale - 3, 7, 7, 255, 220, 140, 255); draw_ellipse(&A.disc, SCREEN_W / 2 + 340, y + 32 * scale - 3, 7, 7, 255, 220, 140, 255); }
    Uint8 a = dim ? 120 : 255;
    draw_text_col(&A.font, s, SCREEN_W / 2, y, scale, 1, sel ? 255 : 200, sel ? 236 : 176, sel ? 180 : 140, a);
}

static void draw_title_screen(void) {
    float t = scene_t;
    if (A.bg.tex) draw_tex(&A.bg, -fmodf(t * 20, A.bg.w - SCREEN_W), 0, (float)A.bg.w, SCREEN_H, 0, 255);
    fill_rect(0, 0, SCREEN_W, SCREEN_H, 14, 6, 4, 165);
    if (A.inky[0].tex) { float s = 1.0f; Tex *k = &A.inky[0];
        draw_tex_tint(k, SCREEN_W - k->w * s - 10, SCREEN_H - k->h * s + 50 + sinf(t * 1.5f) * 8, k->w * s, k->h * s, 0, 150, 130, 120, 255, sinf(t) * 2); }
    float mx = 180, my = 30, mw = 600, mh = 196;
    fill_rect((int)mx, (int)my, (int)mw, (int)mh, 40, 14, 12, 255);
    fill_rect((int)mx + 12, (int)my + 12, (int)mw - 24, (int)mh - 24, 120, 26, 22, 255);
    fill_rect((int)mx + 22, (int)my + 22, (int)mw - 44, (int)mh - 44, 32, 14, 10, 255);
    for (int i = 0; i < 22; i++) {
        int on = ((i + (int)(t * 8)) % 3) != 0; float bx, by;
        if (i < 8) { bx = mx + 30 + i * 77; by = my + 6; } else if (i < 16) { bx = mx + 30 + (i - 8) * 77; by = my + mh - 6; }
        else if (i < 19) { bx = mx + 6; by = my + 50 + (i - 16) * 48; } else { bx = mx + mw - 6; by = my + 50 + (i - 19) * 48; }
        draw_ellipse(&A.disc, bx, by, 8, 8, on ? 255 : 90, on ? 230 : 70, on ? 150 : 50, 255);
        if (on) draw_glow(&A.disc, bx, by, 26, 255, 200, 110, 150);
    }
    draw_text_col(&A.font, "MIDNIGHT", mx + mw / 2, my + 30, 1.6f, 1, 255, 236, 180, 255);
    draw_text_col(&A.font, "MATINEE", mx + mw / 2, my + 100, 1.6f, 1, 255, 210, 120, 255);
    int pc = p1_char < 0 ? 0 : p1_char;
    char r0[64], r1[96], r3[64], r4[64], r5[64];
    snprintf(r0, sizeof r0, "RUNNER:  <  %s  >", CHAR_NAME[pc]);
    if (!has_progress()) snprintf(r1, sizeof r1, "START THE SHOW");
    else snprintf(r1, sizeof r1, "CONTINUE: SHOW %d%s", save.cont_level + 1, save.cont_loop ? " (2ND SHOW)" : "");
    snprintf(r3, sizeof r3, "PLAY STYLE:  <  %s  >", save.style ? "SECURITY CO-OP" : "SOLO");
    snprintf(r4, sizeof r4, "DIFFICULTY:  <  %s  >", DIFF_NAME[save.difficulty]);
    snprintf(r5, sizeof r5, rush_unlocked() ? "BOSS RUSH" : "BOSS RUSH  (LOCKED)");
    const char *items[8] = {r0, r1, "SHOW SELECT", r3, r4, r5, "EXTRAS", "OPTIONS"};
    for (int i = 0; i < 8; i++) menu_row(262 + i * 47, items[i], menu_sel == i, i == 5 && !rush_unlocked(), 0.52f);
    { float bob = sinf(t * 3) * 3; Uint8 *c = (Uint8[3]){OUTFIT[save.outfit].r, OUTFIT[save.outfit].g, OUTFIT[save.outfit].b};
      draw_sprite_tint(&A.hero[(pc + 1) % NCHARS][0][0], 150, 560 - bob, 0.82f, 1, 130, c[0], c[1], c[2]);
      draw_sprite_tint(&A.hero[pc][0][0], 96, 590 + bob, 1.05f, 1, 255, c[0], c[1], c[2]);
      draw_text_col(&A.small, "P1", 96, 600, 0.5f, 1, 255, 220, 150, 255);
      draw_text_col(&A.small, "P2", 150, 570, 0.42f, 1, 220, 200, 170, 150); }
    { char b[48]; snprintf(b, sizeof b, "FILM REELS  %d / 24", save_total_reels()); draw_text_col(&A.small, b, mx + mw / 2, my + mh + 6, 0.48f, 1, 230, 200, 150, 220); }
    { char b[24]; snprintf(b, sizeof b, "SLOT %d", save_slot + 1); draw_text_col(&A.small, b, 24, 18, 0.5f, 0, 230, 200, 150, 200); }
    draw_hint("[DPAD] CHOOSE   [OK] SELECT   [-] SAVE SLOTS"); draw_msg();
}

static void draw_load_card(void) {
    static const char *TIPS[] = {
        "HOLD R TO PLANT YOUR FEET AND AIM IN EIGHT DIRECTIONS.", "FLIP A BREAKER (PRESS UP) TO KEEP THE LIGHTS ON.",
        "THREE FILM REELS ARE HIDDEN IN EVERY SHOW. LOOK UP!", "BREAK CRATES: SOME HIDE A FRESH HARD HAT.",
        "A SECOND PLAYER CAN JUMP IN ANY TIME: PRESS + ON ANOTHER CONTROLLER.",
        "THE GAMEPAD RUNS THE SECURITY CAMERAS: TAP A FEED, PING ENEMIES, HIT THE LIGHTS.",
        "BEAT A SHOW IN UNDER SIX MINUTES WITHOUT A KNOCKOUT FOR AN S RANK.", "IN THE DARK, EVERYTHING GETS FASTER. KEEP THE POWER UP!",
        "PRESS + TO PAUSE. THE SHOW WILL WAIT FOR YOU."};
    int L = ld.pending ? ld.level : level_no;
    draw_curtains(fminf(1, scene_t * 2));
    draw_text_col(&A.font, rush_on ? "BOSS RUSH" : "NOW SHOWING", SCREEN_W / 2, 64, 0.8f, 1, 255, 226, 160, 255);
    float pw = 640, ph = 360, px = SCREEN_W / 2 - pw / 2, py = 128;
    fill_rect((int)px - 18, (int)py - 18, (int)pw + 36, (int)ph + 36, 60, 22, 14, 255);
    fill_rect((int)px - 10, (int)py - 10, (int)pw + 20, (int)ph + 20, 200, 160, 70, 255);
    if (posters[L].tex) draw_tex(&posters[L], px, py, pw, ph, 0, (Uint8)fminf(255, scene_t * 600));
    for (int i = 0; i < 14; i++) { int on = ((i + (int)(scene_t * 8)) % 3) != 0; float bx = px - 4 + i * (pw + 8) / 13.0f;
        draw_ellipse(&A.disc, bx, py - 14, 6, 6, on ? 255 : 90, on ? 230 : 70, on ? 150 : 50, 255);
        draw_ellipse(&A.disc, bx, py + ph + 14, 6, 6, on ? 255 : 90, on ? 230 : 70, on ? 150 : 50, 255); }
    char b[96]; snprintf(b, sizeof b, "SHOW %d: %s", L + 1, LEVELS[L].title);
    draw_text_col(&A.font, b, SCREEN_W / 2, 516, 0.8f, 1, 255, 236, 190, 255);
    draw_text_col(&A.small, LEVELS[L].subtitle, SCREEN_W / 2, 572, 0.55f, 1, 230, 200, 160, 255);
    if (rush_on) { char t2[48]; fmt_time(t2, sizeof t2, (int)(rush_time * 10)); snprintf(b, sizeof b, "BOSS %d OF 8   -   TIME %s", L + 1, t2); }
    else { char t2[32]; fmt_time(t2, sizeof t2, save.best_time[L]);
        snprintf(b, sizeof b, "%s   -   BEST RANK %s   -   BEST TIME %s", DIFF_NAME[difficulty()], GRADE_NAME[save.grade[L]], t2); }
    draw_text_col(&A.small, b, SCREEN_W / 2, 604, 0.46f, 1, 210, 190, 160, 230);
    if (!rush_on) draw_reel_row(SCREEN_W / 2 + 470, 540, save.reels[L], 13);
    draw_text_col(&A.small, TIPS[(L * 7 + save_total_reels()) % (int)(sizeof TIPS / sizeof *TIPS)], SCREEN_W / 2, 636, 0.46f, 1, 255, 210, 140, 220);
    draw_hint(ld.pending ? "LOADING..." : scene_t > 0.8f ? "PRESS [OK]" : "");
}

static void draw_select(void) {
    draw_curtains(1); draw_header("SHOW SELECT");
    for (int s = 0; s < NUM_LEVELS; s++) {
        float w = 268, h = 151, x = 86 + (s % 4) * (w + 24), y = 128 + (s / 4) * (h + 104);
        int on = s == sel_show, un = show_unlocked(s);
        fill_rect((int)x - 8, (int)y - 8, (int)w + 16, (int)h + 16, on ? 230 : 70, on ? 180 : 40, on ? 80 : 30, 255);
        if (un && posters[s].tex) draw_tex(&posters[s], x, y, w, h, 0, 255);
        else { fill_rect((int)x, (int)y, (int)w, (int)h, 24, 14, 12, 255); draw_text_col(&A.font, "?", x + w / 2, y + 36, 1.2f, 1, 120, 90, 70, 255); }
        char b[64]; snprintf(b, sizeof b, "%d. %s", s + 1, un ? LEVELS[s].title : "? ? ?");
        draw_text_col(&A.small, b, x + w / 2, y + h + 12, fit(&A.small, b, 0.5f, w + 10), 1, on ? 255 : 210, on ? 236 : 190, on ? 190 : 160, 255);
        if (un) { draw_reel_row(x + 34, y + h + 58, save.reels[s], 8);
            if (save.grade[s]) draw_grade(x + w - 24, y + h + 58, save.grade[s], 0.42f, 255);
            if (save.cleared[0][s] || save.cleared[1][s]) draw_text_col(&A.small, "CLEARED", x + w / 2 + 4, y + h + 48, 0.4f, 1, 140, 220, 140, 255); }
    }
    { char b[96], t2[32]; fmt_time(t2, sizeof t2, save.best_time[sel_show]);
      if (show_unlocked(sel_show)) snprintf(b, sizeof b, "BEST SCORE %06d    BEST TIME %s", save.best_score[sel_show], t2); else snprintf(b, sizeof b, "LOCKED");
      draw_text_col(&A.small, b, SCREEN_W / 2, 624, fit(&A.small, b, 0.5f, 1000), 1, 240, 210, 170, 255); }
    draw_hint("[DPAD] CHOOSE   [OK] PLAY   [BACK] BACK"); draw_msg();
}

static void draw_options(void) {
    if (opt_back == SC_PLAY) fill_rect(0, 0, SCREEN_W, SCREEN_H, 10, 4, 2, 200); else draw_curtains(1);
    draw_header("OPTIONS");
    char r[8][64];
    snprintf(r[0], 64, "MUSIC VOLUME   <  %d  >", save.music_vol);
    snprintf(r[1], 64, "SOUND EFFECTS   <  %d  >", save.sfx_vol);
    snprintf(r[2], 64, "RUMBLE   <  %s  >", save.rumble ? "ON" : "OFF");
    snprintf(r[3], 64, "BUTTONS   <  %s  >", save.layout ? "A JUMPS / B SHOOTS" : "B JUMPS / A SHOOTS");
    snprintf(r[4], 64, "SCREEN SHAKE   <  %s  >", save.shake ? "ON" : "OFF");
    snprintf(r[5], 64, "LANGUAGE   <  %s  >", save.language ? "ESPANOL" : "ENGLISH");
    snprintf(r[6], 64, erase_armed ? "PRESS AGAIN TO ERASE EVERYTHING!" : "ERASE SAVE DATA");
    snprintf(r[7], 64, "BACK");
    int n = 0;
    for (int i = 0; i < 8; i++) { if (opt_back == SC_PLAY && i == 6) continue;
        int sel = opt_sel == n; menu_row(150 + n * 56, r[i], sel, 0, 0.56f);
        if (i < 2) { float bx = SCREEN_W / 2 - 150, by = 150 + n * 56 + 42; fill_rect((int)bx, (int)by, 300, 6, 50, 30, 24, 255);
            fill_rect((int)bx, (int)by, (int)(300 * (i ? save.sfx_vol : save.music_vol) / 10), 6, 255, 200, 90, 255); }
        n++; }
    draw_hint("[DPAD] CHANGE   [BACK] BACK"); draw_msg();
}

static void draw_pause(void) {
    fill_rect(0, 0, SCREEN_W, SCREEN_H, 10, 4, 2, 175);
    fill_rect(SCREEN_W / 2 - 300, 150, 600, 400, 40, 14, 12, 245); fill_rect(SCREEN_W / 2 - 290, 160, 580, 380, 26, 10, 8, 255);
    draw_text_col(&A.font, "INTERMISSION", SCREEN_W / 2, 186, 0.95f, 1, 255, 226, 160, 255);
    const char *it[4] = {"RESUME", rush_on ? "RESTART THE BOSS RUSH" : "RESTART THIS SHOW", "OPTIONS", rush_on ? "QUIT THE BOSS RUSH" : "QUIT TO TITLE"};
    for (int i = 0; i < 4; i++) menu_row(258 + i * 56, it[i], pause_sel == i, 0, 0.56f);
    char b[96], t2[32]; fmt_time(t2, sizeof t2, (int)((rush_on ? rush_time : show_time) * 10));
    snprintf(b, sizeof b, "SHOW %d: %s   -   %s", level_no + 1, LEVELS[level_no].title, t2);
    draw_text_col(&A.small, b, SCREEN_W / 2, 482, 0.45f, 1, 220, 200, 170, 230);
    if (!rush_on) draw_reel_row(SCREEN_W / 2, 524, save.reels[level_no] | reels_run, 12);
    draw_rich(&A.small, "[+] RESUME   [OK] SELECT", SCREEN_W / 2, 600, 0.45f, 1, 220, 200, 170, 200);
}

static void draw_extras(void) {
    draw_curtains(1); draw_header("EXTRAS");
    char b[7][64]; int nb = 0; for (int i = 0; i < 32; i++) nb += (save.badge[i / 8] >> (i % 8)) & 1;
    snprintf(b[0], 64, "GALLERY"); snprintf(b[1], 64, "JUKEBOX"); snprintf(b[2], 64, "WARDROBE");
    snprintf(b[3], 64, "STAGEHAND BADGES  %d / 20", nb); snprintf(b[4], 64, "WATCH THE INTRO"); snprintf(b[5], 64, "CREDITS"); snprintf(b[6], 64, "BACK");
    for (int i = 0; i < 7; i++) menu_row(150 + i * 58, b[i], extras_sel == i, 0, 0.6f);
    char r[64]; snprintf(r, sizeof r, "FILM REELS FOUND:  %d / 24", save_total_reels());
    draw_text_col(&A.small, r, SCREEN_W / 2, 590, 0.6f, 1, 255, 210, 140, 255);
    draw_hint("[OK] OPEN   [BACK] BACK"); draw_msg();
}

static void draw_gallery(void) {
    if (gal_view) {
        fill_rect(0, 0, SCREEN_W, SCREEN_H, 8, 4, 4, 255);
        int s = gal_sel / 3;
        if (gal_sel % 3 == 0) {
            if (posters[s].tex) draw_tex(&posters[s], 0, 0, SCREEN_W, SCREEN_H, 0, 110);
            Tex *t = boss_tex(s); if (t->tex) { float k = fminf(560.0f / t->h, 900.0f / t->w); draw_tex(t, SCREEN_W / 2 - t->w * k / 2, 70, t->w * k, t->h * k, 0, 255); }
        } else if (gal_tex.tex) draw_tex(&gal_tex, 0, 0, SCREEN_W, SCREEN_H, 0, 255);
        fill_rect(0, SCREEN_H - 70, SCREEN_W, 70, 10, 4, 4, 210);
        char b[128]; draw_text_col(&A.small, gal_caption(gal_sel, b, sizeof b), SCREEN_W / 2, SCREEN_H - 56, 0.55f, 1, 255, 226, 170, 255);
        draw_rich(&A.small, "[DPAD] BROWSE   [BACK] BACK", SCREEN_W / 2, SCREEN_H - 28, 0.42f, 1, 200, 180, 150, 200);
        return;
    }
    draw_curtains(1); draw_header("GALLERY");
    for (int i = 0; i < 24; i++) {
        int s = i / 3, k = i % 3; float w = 132, h = 96, x = 60 + s * 148, y = 150 + k * 128;
        int on = i == gal_sel, un = gal_unlocked(i);
        fill_rect((int)x - 5, (int)y - 5, (int)w + 10, (int)h + 10, on ? 240 : 60, on ? 190 : 34, on ? 80 : 26, 255);
        if (un && posters[s].tex) { SDL_Rect src = {k == 0 ? 320 : k == 1 ? 0 : 160, k == 2 ? 120 : 40, 320, 232}; draw_tex_src(&posters[s], &src, x, y, w, h, 255); }
        else { fill_rect((int)x, (int)y, (int)w, (int)h, 22, 12, 10, 255); draw_reel_icon(x + w / 2, y + h / 2, 22, 0, 200); }
        if (k == 0) { char b[8]; snprintf(b, sizeof b, "%d", s + 1); draw_text_col(&A.small, b, x + w / 2, y - 34, 0.5f, 1, 230, 200, 150, 255); }
    }
    char b[128];
    if (gal_unlocked(gal_sel)) gal_caption(gal_sel, b, sizeof b); else snprintf(b, sizeof b, "UNDEVELOPED: FIND FILM REEL %d IN SHOW %d", gal_sel % 3 + 1, gal_sel / 3 + 1);
    draw_text_col(&A.small, b, SCREEN_W / 2, 560, 0.5f, 1, 240, 210, 170, 255);
    draw_hint("[DPAD] CHOOSE   [OK] VIEW   [BACK] BACK"); draw_msg();
}

static void draw_jukebox(void) {
    draw_curtains(1); draw_header("JUKEBOX");
    int top = juke_sel - 4; if (top > ntracks - 9) top = ntracks - 9; if (top < 0) top = 0;
    for (int r = 0; r < 9 && top + r < ntracks; r++) { int i = top + r, un = track_unlocked(i);
        char b[80]; snprintf(b, sizeof b, "%s%s", i == juke_playing ? "> " : "", un ? tracks[i].label : "? ? ?");
        menu_row(150 + r * 50, b, i == juke_sel, !un, 0.46f); }
    if (juke_playing >= 0) { char b[96]; snprintf(b, sizeof b, "NOW PLAYING: %s", tracks[juke_playing].label);
        draw_text_col(&A.small, b, SCREEN_W / 2, 612, 0.5f, 1, 255, 210, 140, 255);
        for (int k = 0; k < 12; k++) { float hh = 8 + 22 * fabsf(sinf(scene_t * (3 + k * 0.7f) + k)); fill_rect(SCREEN_W / 2 - 150 + k * 26, 600 - (int)hh, 16, (int)hh, 255, 190, 90, 200); } }
    draw_hint("[OK] PLAY   [BACK] BACK"); draw_msg();
}

static void draw_wardrobe(void) {
    draw_curtains(1); draw_header("WARDROBE");
    int total = save_total_reels();
    for (int i = 0; i < 5; i++) { char b[80]; int un = total >= OUTFIT[i].need;
        if (un) snprintf(b, sizeof b, "%s%s", OUTFIT[i].name, save.outfit == i ? "   (WEARING)" : "");
        else snprintf(b, sizeof b, "%s  -  %d FILM REELS", OUTFIT[i].name, OUTFIT[i].need);
        menu_row(170 + i * 62, b, ward_sel == i, !un, 0.52f); }
    for (int c = 0; c < NCHARS; c++) { float x = c == 0 ? 170 : SCREEN_W - 170;
        draw_ellipse(&A.disc, x, 560, 70, 12, 0, 0, 0, 120);
        draw_sprite_tint(&A.hero[c][0][0], x, 560, 1.3f, c == 0, 255, OUTFIT[ward_sel].r, OUTFIT[ward_sel].g, OUTFIT[ward_sel].b); }
    char b[64]; snprintf(b, sizeof b, "FILM REELS FOUND:  %d / 24", total); draw_text_col(&A.small, b, SCREEN_W / 2, 512, 0.55f, 1, 255, 210, 140, 255);
    draw_hint("[OK] WEAR   [BACK] BACK"); draw_msg();
}

static void draw_intro(void) {
    fill_rect(0, 0, SCREEN_W, SCREEN_H, 0, 0, 0, 255);
    Tex *t = &intro_tex[intro_panel]; float k = scene_t / INTRO_PANEL_T;
    if (t->tex) { float z = 1.0f + 0.08f * k, w = SCREEN_W * z, h = SCREEN_H * z, dx = (intro_panel & 1 ? -1 : 1) * 30 * k;
        draw_tex(t, SCREEN_W / 2 - w / 2 + dx, SCREEN_H / 2 - h / 2, w, h, 0, 255); }
    float fade = fminf(fminf(scene_t / 0.6f, (INTRO_PANEL_T - scene_t) / 0.6f), 1);
    fill_rect(0, 0, SCREEN_W, SCREEN_H, 0, 0, 0, (Uint8)(255 * (1 - clampf(fade, 0, 1))));
    fill_rect(0, SCREEN_H - 130, SCREEN_W, 130, 8, 4, 2, 200);
    const char *s = TR(INTRO_TEXT[intro_panel]); int n = (int)(scene_t * 34); char b[160];
    snprintf(b, sizeof b, "%.*s", n < (int)strlen(s) ? n : (int)strlen(s), s);
    /* wrap into two lines at a space past the middle */
    int len = (int)strlen(b), cut = -1; if (len > 48) for (int i = 44; i < len; i++) if (b[i] == ' ') { cut = i; break; }
    if (cut > 0) { b[cut] = 0; draw_text_col(&A.small, b, SCREEN_W / 2, SCREEN_H - 112, 0.62f, 1, 255, 236, 200, 255); draw_text_col(&A.small, b + cut + 1, SCREEN_W / 2, SCREEN_H - 76, 0.62f, 1, 255, 236, 200, 255); }
    else draw_text_col(&A.small, b, SCREEN_W / 2, SCREEN_H - 96, 0.62f, 1, 255, 236, 200, 255);
    draw_rich(&A.small, "[OK] NEXT   [+] SKIP", SCREEN_W - 24, 16, 0.42f, 2, 220, 200, 170, 170);
}

static void draw_credits(void) {
    fill_rect(0, 0, SCREEN_W, SCREEN_H, 12, 4, 4, 255);
    for (int i = 0; i < 24; i++) { float x = i * 54 + 20; int on = ((i + (int)(scene_t * 6)) % 3) != 0;
        draw_ellipse(&A.disc, x, 14, 6, 6, on ? 255 : 80, on ? 220 : 60, on ? 140 : 40, 255); draw_ellipse(&A.disc, x, SCREEN_H - 14, 6, 6, on ? 255 : 80, on ? 220 : 60, on ? 140 : 40, 255); }
    float y = credits_y();
    for (int i = 0; i < NCRED; i++, y += 52) {
        if (y < -120 || y > SCREEN_H + 40) continue;
        const char *s = CREDITS[i];
        if (s[0] == '#') draw_text_col(&A.font, s + 1, SCREEN_W / 2, y, 0.95f, 1, 255, 226, 150, 255);
        else if (s[0] == '@') draw_text_col(&A.font, s + 1, SCREEN_W / 2, y, 0.55f, 1, 255, 240, 210, 255);
        else if (s[0] == '*') { int L = s[1] - '0'; char b[96]; snprintf(b, sizeof b, "SHOW %d  %s  -  %s", L + 1, LEVELS[L].title, LEVELS[L].boss_name);
            draw_text_col(&A.small, b, SCREEN_W / 2, y + 6, 0.55f, 1, 240, 210, 170, 255);
            Tex *t = boss_tex(L); if (t->tex) { float k = 90.0f / t->h; float bx = (L & 1) ? SCREEN_W - 170 : 170; draw_tex(t, bx - t->w * k / 2, y - 30, t->w * k, 90, L & 1, 255); } }
        else draw_text_col(&A.small, s, SCREEN_W / 2, y + 6, 0.5f, 1, 210, 180, 150, 255);
    }
    draw_rich(&A.small, "HOLD [OK] FAST FORWARD   [+] SKIP", SCREEN_W - 24, 26, 0.4f, 2, 200, 180, 150, 160);
}

static void draw_rush_done(void) {
    draw_curtains(1); draw_header("BOSS RUSH COMPLETE!");
    char b[64]; fmt_time(b, sizeof b, (int)(rush_time * 10));
    draw_text_col(&A.font, b, SCREEN_W / 2, 240, 1.4f, 1, 255, 236, 180, 255);
    char r[64], t2[32]; fmt_time(t2, sizeof t2, save.rush_best); snprintf(r, sizeof r, "BEST  %s", t2);
    draw_text_col(&A.small, r, SCREEN_W / 2, 360, 0.7f, 1, 240, 210, 170, 255);
    if (rush_new && (int)(scene_t * 3) % 2) draw_text_col(&A.font, "NEW RECORD!", SCREEN_W / 2, 420, 0.8f, 1, 255, 200, 80, 255);
    for (int s = 0; s < NUM_LEVELS; s++) { Tex *t = boss_tex(s); if (!t->tex) continue; float k = 110.0f / t->h; float x = 120 + s * 148;
        draw_tex_tint(t, x - t->w * k / 2, 500, t->w * k, 110, 0, 90, 80, 80, 255, 0); draw_text_col(&A.font, "X", x, 520, 1.0f, 1, 220, 40, 30, 230); }
    if (scene_t > 2) draw_hint("PRESS [OK]");
}

static void draw_reels_world(float ox, float oy) {
    if (rush_on) return;
    for (int k = 0; k < 3; k++) { ReelSpot *r = &reel_spot[k]; if (!r->live) continue;
        float x = sx(r->x) + ox, y = r->y + oy + sinf(scene_t * 3 + k) * 6;
        if (x < -60 || x > SCREEN_W + 60) continue;
        int owned = (save.reels[level_no] >> k) & 1;
        draw_reel_icon(x, y, 22, 1, owned ? 110 : 255);
    }
}
static void update_reels(void) {
    if (rush_on || demo_on) return;
    for (int k = 0; k < 3; k++) { ReelSpot *r = &reel_spot[k]; if (!r->live) continue;
        for (int i = 0; i < 2; i++) { Player *p = &pl[i]; if (!p->active || !p->alive || p->hidden) continue;
            if (fabsf(p->x - r->x) < 48 && p->y - 130 < r->y + 24 && p->y > r->y - 30) {
                r->live = 0; reels_run |= 1 << k; spark_burst(r->x, r->y, 30, 255, 220, 120); ring(r->x, r->y, 90, 255, 220, 140);
                if (!((save.reels[level_no] >> k) & 1)) {
                    save.reels[level_no] |= (unsigned char)(1 << k); award(4); if (save_total_reels() >= 12) award(5); if (save_total_reels() >= 24) award(6);
                    game_save(); sfx("reel"); vo(p->chr, "reel"); rumble_pad(p->pad, 0.4f, 180);
                    char b[64]; snprintf(b, sizeof b, "FILM REEL FOUND!  %d / 3 IN THIS SHOW", popcount3(save.reels[level_no])); message(b, 2.2f);
                } else { sfx("coin"); message("YOU ALREADY HAVE THIS REEL", 1.2f); }
                break;
            } }
    }
}

/* Show 1's opening stretch teaches the controls on painted signs (they follow the button layout option) */
static void draw_tutorial_signs(float ox, float oy) {
    if (level_no != L_LOBBY || loop_no || rush_on) return;
    int closet_tx = -1; for (int x = 0; x < LW; x++) if (map[12][x] == 'C') { closet_tx = x; break; }
    struct { int tx; const char *a, *b; } sg[9] = {{3, "MOVE", "[DPAD] OR STICK"}, {6, "JUMP", "[JUMP]"}, {12, "BIG JUMP", "HOLD [JUMP]"},
        {16, "SHOOT", "[SHOOT] OR [Y]"}, {20, "AIM", "HOLD [R] + [DPAD]"}, {28, "LIGHTS ON", "UP AT A BREAKER"}, {32, "SWAP GUNS", "[L]"},
        {37, "PAUSE", "[+]"}, {closet_tx > 2 ? closet_tx - 3 : -1, "HIDE", "UP AT A CLOSET"}};
    for (int i = 0; i < 9; i++) { if (sg[i].tx < 0) continue;
        float x = sx(sg[i].tx * TILE + 24) + ox, y = FLOOR_Y + oy; if (x < -120 || x > SCREEN_W + 120) continue;
        fill_rect((int)x - 5, (int)y - 150, 10, 150, 70, 44, 26, 255);
        fill_rect((int)x - 68, (int)y - 222, 136, 74, 50, 30, 18, 255); fill_rect((int)x - 63, (int)y - 217, 126, 64, 128, 86, 48, 255);
        draw_text_col(&A.small, sg[i].a, x, y - 211, fit(&A.small, sg[i].a, 0.42f, 118), 1, 255, 236, 190, 255);
        { float sc = 0.36f, w = rich_width(&A.small, sg[i].b, sc); if (w > 120) sc *= 120 / w; draw_rich(&A.small, sg[i].b, x, y - 185, sc, 1, 40, 20, 10, 255); }
    }
}

/* ------------------------------------------------------------ the game step for every non-play scene */
static int step_front(const Input pads[MAX_PADS], unsigned anyp, unsigned anyh) {
    (void)pads;
    switch (scene) {
    case SC_LOGO: if (scene_t > 3.3f || (scene_t > 0.5f && anyp)) to_press(); return 1;
    case SC_PRESS: if (anyp & (OK_BITS() | B_START)) { sfx("confirm"); open_slots(); } else if (scene_t > 25) start_demo(); return 1;
    case SC_SLOTS: step_slots(anyp); return 1;
    case SC_BADGES: step_badges(anyp); return 1;
    case SC_TITLE: step_title(anyp); return 1;
    case SC_SELECT: step_select(anyp); return 1;
    case SC_OPTIONS: step_options(anyp); return 1;
    case SC_EXTRAS: step_extras(anyp); return 1;
    case SC_GALLERY: step_gallery(anyp); return 1;
    case SC_JUKEBOX: step_jukebox(anyp); return 1;
    case SC_WARDROBE: step_wardrobe(anyp); return 1;
    case SC_INTRO: step_intro(anyp); return 1;
    case SC_CREDITS: step_credits(anyp, anyh); return 1;
    case SC_RUSHDONE: if (scene_t > 2 && (anyp & OK_BITS())) go_title(); return 1;
    case SC_LOAD:
        if (ld.pending) { if (scene_t > 0.05f) { float t = scene_t; finish_load(); scene = SC_LOAD; scene_t = t; } return 1; }
        if (scene_t > 2.6f || (scene_t > 0.8f && (anyp & OK_BITS()))) {
            scene = SC_PLAY; scene_t = 0; char b[64];
            if (!demo_on) vo(pl[0].chr, "go");
            if (rush_on) snprintf(b, sizeof b, "BOSS %d: %s", level_no + 1, LEVELS[level_no].boss_name);
            else if (loop_no && level_no == 0 && ld.keep == 2) snprintf(b, sizeof b, "THE SECOND SHOW: EVERYTHING IS FASTER AND MEANER");
            else snprintf(b, sizeof b, "SHOW %d: %s", level_no + 1, LEVELS[level_no].title);
            message(b, 2.5f);
        }
        return 1;
    case SC_END:
        if (scene_t > 6.0f && (anyp & OK_BITS())) {
            if (!loop_no) { save.seen_ending = 1; save.cont_level = 0; save.cont_loop = 1; game_save(); start_credits(1); }
            else { save.true_ending = 1; save.cont_level = 0; save.cont_loop = 0; award(3); game_save(); start_credits(2); }
        }
        return 1;
    case SC_OVER:
        if (scene_t > 2.0f) {
            if (anyp & (B_UP | B_DOWN)) { over_sel ^= 1; sfx("menu"); }
            if (anyp & OK_BITS()) { sfx("menu");
                if (over_sel == 1) go_title();
                else if (rush_on) { rush_time = 0; queue_show(0, 0); }
                else queue_show(level_no, 0); }
        }
        return 1;
    case SC_CLEAR:
        { static int said = 0; if (scene_t < 0.1f) said = 0;
          if (!said && scene_t >= after_len() + 0.2f && !rush_on && !demo_on) { said = 1; sfx("ann_clear"); } }
        if (scene_t < after_len()) { if (scene_t > 0.6f && (anyp & OK_BITS())) scene_t = after_len(); return 1; }
        if (scene_t > after_len() + 2.0f && (anyp & OK_BITS())) {
            if (rush_on) {
                if (level_no + 1 < NUM_LEVELS) queue_show(level_no + 1, 1);
                else { int tt = (int)(rush_time * 10); rush_new = !save.rush_best || tt < save.rush_best; if (rush_new) save.rush_best = tt; award(14); if (rush_time < 480) award(15); game_save();
                       scene = SC_RUSHDONE; scene_t = 0; music_play("mm_ending", 0); }
            }
            else if (level_no + 1 >= NUM_LEVELS) { scene = SC_END; scene_t = 0; music_play("mm_ending", 0); }
            else queue_show(level_no + 1, 1);
        }
        if (scene == SC_CLEAR) update_proj();
        return 1;
    }
    return 0;
}
static int draw_front(void) {
    switch (scene) {
    case SC_LOGO: draw_logo(); return 1;
    case SC_PRESS: draw_press(); break;
    case SC_SLOTS: draw_slots(); break;
    case SC_BADGES: draw_badges(); break;
    case SC_TITLE: draw_title_screen(); break;
    case SC_SELECT: draw_select(); break;
    case SC_OPTIONS: if (opt_back == SC_PLAY) return 0; draw_options(); break;
    case SC_EXTRAS: draw_extras(); break;
    case SC_GALLERY: draw_gallery(); break;
    case SC_JUKEBOX: draw_jukebox(); break;
    case SC_WARDROBE: draw_wardrobe(); break;
    case SC_INTRO: draw_intro(); break;
    case SC_CREDITS: draw_credits(); break;
    case SC_RUSHDONE: draw_rush_done(); break;
    case SC_LOAD: draw_load_card(); break;
    default: return 0;
    }
    draw_post(); return 1;
}
/* the end-of-show card: rank, time, reels */
static void draw_clear_card(void) {
    char b[96]; float st = scene_t - after_len();
    if (rush_on) {
        snprintf(b, sizeof b, "%s DOWN!", LEVELS[level_no].boss_name); draw_text_col(&A.font, b, SCREEN_W / 2, 220, 1.1f, 1, 255, 230, 150, 255);
        snprintf(b, sizeof b, "BOSS %d OF 8", level_no + 1); draw_text_col(&A.small, b, SCREEN_W / 2, 320, 0.7f, 1, 240, 210, 170, 255);
        char t2[32]; fmt_time(t2, sizeof t2, (int)(rush_time * 10)); snprintf(b, sizeof b, "TIME %s", t2); draw_text_col(&A.font, b, SCREEN_W / 2, 370, 0.8f, 1, 255, 240, 210, 255);
        return;
    }
    snprintf(b, sizeof b, "%s CLEARED!", LEVELS[level_no].title); draw_text_col(&A.font, b, SCREEN_W / 2, 120, 1.1f, 1, 255, 230, 150, 255);
    float k = clampf(st - 0.4f, 0, 1);
    char t2[32]; fmt_time(t2, sizeof t2, (int)(show_time * 10));
    const char *lab[4] = {"SCORE", "TIME", "KNOCKOUTS", "FILM REELS"};
    char val[4][32]; snprintf(val[0], 32, "%06d", score - show_score0); snprintf(val[1], 32, "%s", t2); snprintf(val[2], 32, "%d", show_deaths); snprintf(val[3], 32, " ");
    for (int i = 0; i < 4; i++) { float y = 230 + i * 56; Uint8 a = (Uint8)(255 * clampf((st - 0.3f - i * 0.25f) * 4, 0, 1));
        draw_text_col(&A.small, lab[i], SCREEN_W / 2 - 300, y, 0.62f, 0, 230, 200, 160, a);
        draw_text_col(&A.font, val[i], SCREEN_W / 2 + 40, y - 6, 0.62f, 0, 255, 240, 210, a);
        if ((i == 0 && (show_newbest & 2)) || (i == 1 && (show_newbest & 4))) draw_text_col(&A.small, "BEST!", SCREEN_W / 2 + 196, y, 0.5f, 0, 255, 200, 80, a); }
    if (st > 1.3f) draw_reel_row(SCREEN_W / 2 + 100, 418, reels_run | save.reels[level_no], 14);
    if (st > 1.6f) { float pop = 1.0f + 0.6f * fmaxf(0, 1 - (st - 1.6f) * 4);
        draw_text_col(&A.small, "RANK", SCREEN_W / 2 + 450, 210, 0.6f, 1, 230, 200, 160, 255);
        draw_grade(SCREEN_W / 2 + 450, 300, show_grade, 1.5f * pop * k, 255);
        if (show_newbest & 1) draw_text_col(&A.small, "NEW BEST RANK!", SCREEN_W / 2 + 450, 380, 0.45f, 1, 255, 200, 80, 255); }
    if (level_no + 1 < NUM_LEVELS) { snprintf(b, sizeof b, "NEXT: SHOW %d - %s", level_no + 2, LEVELS[level_no + 1].title); draw_text_col(&A.small, b, SCREEN_W / 2, 490, 0.6f, 1, 240, 210, 170, 255); }
    else draw_text_col(&A.small, "THE FINAL CURTAIN...", SCREEN_W / 2, 490, 0.6f, 1, 240, 210, 170, 255);
}

/* ==================================================================================================
   RETAIL POLISH: studio logo, press start + attract demo, save slots, iris wipes, button icons,
   boss entrances and aftermath cards
   ================================================================================================== */
static int demo_show, slot_sel, slot_erase_armed, slot_has[SAVE_SLOTS], iris_last = -1;
static float iris_k = 1;
static unsigned demo_prev;
static SaveData slot_peek[SAVE_SLOTS];
#define AFTER_T 3.6f   /* how long the aftermath card holds before the score card */
static const char *AFTER_LINE[NUM_LEVELS] = {
    "UNCLE INKY'S BATON HAS BEEN RETIRED.", "THE KITCHEN IS CLOSED. PERMANENTLY.", "THAT'S A WRAP!", "STOP THE PRESSES!",
    "LOST: ONE COLLECTOR. NOBODY CAME TO CLAIM HIM.", "SEVEN YEARS BAD LUCK.", "HIS NAME IS NO LONGER IN LIGHTS.", "THE INK DRAINS AWAY... OR DOES IT?"};

static void game_save(void) { if (!demo_on) save_write(); }
static float after_len(void) { return (!rush_on && aftermath_tex.tex) ? AFTER_T : 0; }

/* ---- button icons: "[A] SELECT  [B] BACK" draws real buttons (tokens [OK] [BACK] [JUMP] [SHOOT] follow the layout) ---- */
static const char *icon_label(const char *tok, int n, char *out) {
    char t[8] = {0}; snprintf(t, sizeof t, "%.*s", n < 7 ? n : 7, tok);
    if (!strcmp(t, "OK")) return "A";
    if (!strcmp(t, "BACK")) return "B";
    if (!strcmp(t, "JUMP")) return save.layout ? "A" : "B";
    if (!strcmp(t, "SHOOT")) return save.layout ? "B" : "A";
    snprintf(out, 8, "%s", t); return out;
}
static float icon_w(const char *lab, float h) {
    if (!strcmp(lab, "DPAD")) return h * 1.05f;
    if (!strcmp(lab, "L") || !strcmp(lab, "R") || !strcmp(lab, "ZL") || !strcmp(lab, "ZR")) return h * 1.5f;
    return h * 0.95f;
}
static void draw_icon(const char *lab, float x, float cy, float h, Uint8 a) {
    float r = h * 0.46f;
    if (!strcmp(lab, "DPAD")) {
        float cx = x + h * 0.52f, w = h * 0.3f, l = h * 0.96f;
        fill_rect((int)(cx - w / 2 - 2), (int)(cy - l / 2 - 2), (int)(w + 4), (int)(l + 4), 20, 14, 10, a);
        fill_rect((int)(cx - l / 2 - 2), (int)(cy - w / 2 - 2), (int)(l + 4), (int)(w + 4), 20, 14, 10, a);
        fill_rect((int)(cx - w / 2), (int)(cy - l / 2), (int)w, (int)l, 90, 84, 80, a); fill_rect((int)(cx - l / 2), (int)(cy - w / 2), (int)l, (int)w, 90, 84, 80, a);
        draw_ellipse(&A.disc, cx, cy, w * 0.3f, w * 0.3f, 50, 46, 44, a);
        return;
    }
    int wide = !strcmp(lab, "L") || !strcmp(lab, "R") || !strcmp(lab, "ZL") || !strcmp(lab, "ZR");
    if (wide) { float w = h * 1.4f, x0 = x + h * 0.05f;
        fill_rect((int)x0, (int)(cy - r), (int)w, (int)(r * 2), 20, 14, 10, a);
        fill_rect((int)x0 + 2, (int)(cy - r) + 2, (int)w - 4, (int)(r * 2) - 4, 92, 86, 82, a);
        draw_text_col(&A.small, lab, x0 + w / 2, cy - r * 0.95f, r * 1.7f / 36.0f, 1, 250, 246, 236, a);
        return; }
    float cx = x + h * 0.47f;
    int plusminus = !strcmp(lab, "+") || !strcmp(lab, "-");
    draw_ellipse(&A.disc, cx, cy, r + 2, r + 2, 20, 14, 10, a);
    Uint8 cr = 92, cg = 86, cb = 82;
    if (!strcmp(lab, "A")) { cr = 196; cg = 60; cb = 50; } else if (!strcmp(lab, "B")) { cr = 214; cg = 168; cb = 52; }
    else if (!strcmp(lab, "X")) { cr = 60; cg = 110; cb = 196; } else if (!strcmp(lab, "Y")) { cr = 70; cg = 150; cb = 80; }
    draw_ellipse(&A.disc, cx, cy, plusminus ? r * 0.8f : r, plusminus ? r * 0.8f : r, cr, cg, cb, a);
    if (plusminus) { fill_rect((int)(cx - r * 0.45f), (int)(cy - r * 0.1f), (int)(r * 0.9f), (int)(r * 0.2f) + 1, 250, 246, 236, a);
        if (lab[0] == '+') fill_rect((int)(cx - r * 0.1f), (int)(cy - r * 0.45f), (int)(r * 0.2f) + 1, (int)(r * 0.9f), 250, 246, 236, a); }
    else draw_text_col(&A.small, lab, cx, cy - r * 0.98f, r * 1.75f / 36.0f, 1, 250, 246, 236, a);
}
static float rich_width(Font *f, const char *s, float scale) {
    float w = 0, h = f->size * scale * 1.3f; char seg[160]; int n = 0;
    for (const char *p = s; ; p++) {
        if (*p == '[' || !*p) { seg[n] = 0; if (n) w += text_width(f, seg, scale); n = 0; if (!*p) break;
            const char *e = strchr(p, ']'); if (!e) break; char tmp[8]; w += icon_w(icon_label(p + 1, (int)(e - p - 1), tmp), h) + h * 0.12f; p = e; continue; }
        if (n < 158) seg[n++] = *p;
    }
    return w;
}
/* centered: 0 left, 1 centre, 2 right */
static void draw_rich(Font *f, const char *s, float x, float y, float scale, int centered, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    float w = rich_width(f, s, scale), h = f->size * scale * 1.3f; Uint8 ia = a < 210 ? 210 : a;
    float cx = centered == 1 ? x - w / 2 : centered == 2 ? x - w : x, cy = y + f->size * scale * 0.52f;
    char seg[160]; int n = 0;
    for (const char *p = s; ; p++) {
        if (*p == '[' || !*p) { seg[n] = 0; if (n) { draw_text_col(f, seg, cx, y, scale, 0, r, g, b, a); cx += text_width(f, seg, scale); } n = 0; if (!*p) break;
            const char *e = strchr(p, ']'); if (!e) break; char tmp[8]; const char *lab = icon_label(p + 1, (int)(e - p - 1), tmp);
            draw_icon(lab, cx + h * 0.06f, cy, h, ia); cx += icon_w(lab, h) + h * 0.12f; p = e; continue; }
        if (n < 158) seg[n++] = *p;
    }
}

/* ---- iris wipe: every new screen opens from a circle, old-cartoon style ---- */
static Tex make_iris(void) {
    int w = 256; SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, w, 32, SDL_PIXELFORMAT_RGBA32);
    Uint32 *px = (Uint32 *)s->pixels;
    for (int y = 0; y < w; y++) for (int x = 0; x < w; x++) {
        float dx = x - w / 2.0f + 0.5f, dy = y - w / 2.0f + 0.5f, d = sqrtf(dx * dx + dy * dy);
        px[y * (s->pitch / 4) + x] = SDL_MapRGBA(s->format, 0, 0, 0, (Uint8)(255 * clampf(d - 123, 0, 1)));
    }
    Tex t = tex_from_surface(s); SDL_FreeSurface(s); return t;
}
static int iris_skip(int from, int to) {   /* overlays that sit on the same picture don't get a wipe */
    if ((from == SC_PLAY && (to == SC_OPTIONS || to == SC_CLEAR || to == SC_OVER)) || (from == SC_OPTIONS && to == SC_PLAY)) return 1;
    if (from == SC_LOAD && to == SC_PLAY) return 0;
    return from < 0;
}
static void iris_track(void) {
    if (scene != iris_last) { if (!iris_skip(iris_last, scene)) iris_k = 0; iris_last = scene; }
    if (iris_k < 1) iris_k = fminf(1, iris_k + DT * 2.1f);
}
static void draw_iris(void) {
    if (iris_k >= 1 || !iris_tex.tex) return;
    float e = 1 - (1 - iris_k) * (1 - iris_k), R = 760 * e;
    float cx = SCREEN_W / 2, cy = SCREEN_H / 2;
    if (scene == SC_PLAY && pl[0].active) { cx = clampf(sx(pl[0].x), 80, SCREEN_W - 80); cy = clampf(pl[0].y - 70, 80, SCREEN_H - 80); }
    if (R < 2) { fill_rect(0, 0, SCREEN_W, SCREEN_H, 0, 0, 0, 255); return; }
    float S = R * 256 / 123, x0 = cx - S / 2, y0 = cy - S / 2;
    draw_tex(&iris_tex, x0, y0, S, S, 0, 255);
    if (y0 > 0) fill_rect(0, 0, SCREEN_W, (int)y0 + 1, 0, 0, 0, 255);
    if (y0 + S < SCREEN_H) fill_rect(0, (int)(y0 + S) - 1, SCREEN_W, SCREEN_H, 0, 0, 0, 255);
    if (x0 > 0) fill_rect(0, 0, (int)x0 + 1, SCREEN_H, 0, 0, 0, 255);
    if (x0 + S < SCREEN_W) fill_rect((int)(x0 + S) - 1, 0, SCREEN_W, SCREEN_H, 0, 0, 0, 255);
}

/* ---- boot: studio logo, then press start ---- */
static void to_press(void) { static int said; scene = SC_PRESS; scene_t = 0; music_play("mm_title", 1); sfx("iris"); if (!said) { said = 1; sfx("ann_title"); } }
static void draw_logo(void) {
    fill_rect(0, 0, SCREEN_W, SCREEN_H, 0, 0, 0, 255);
    float t = scene_t, a = clampf(fminf(t / 0.5f, (3.3f - t) / 0.5f), 0, 1);
    int flick = t < 0.6f && (int)(t * 30) % 3 == 0;   /* the projector stutters on */
    if (ego_tex.tex && !flick) { float z = 1.0f + 0.03f * t, w = SCREEN_W * z, h = SCREEN_H * z;
        draw_tex(&ego_tex, SCREEN_W / 2 - w / 2, SCREEN_H / 2 - h / 2, w, h, 0, (Uint8)(255 * a)); }
    for (int k = 0; k < 3; k++) fill_rect((int)(frand() * SCREEN_W), 0, 2, SCREEN_H, 255, 240, 210, (Uint8)(50 * a));   /* scratches */
}
static void draw_press(void) {
    float t = scene_t;
    if (keyart_tex.tex) { float z = 1.04f + 0.02f * sinf(t * 0.3f), w = SCREEN_W * z, h = SCREEN_H * z; draw_tex(&keyart_tex, SCREEN_W / 2 - w / 2, SCREEN_H / 2 - h / 2, w, h, 0, 255); }
    fill_rect(0, 0, SCREEN_W, 140, 10, 2, 2, 120);
    draw_text_col(&A.font, "MIDNIGHT", 270, 34, 1.25f, 1, 255, 232, 170, 255);
    draw_text_col(&A.font, "MATINEE", 1010, 34, 1.25f, 1, 255, 206, 110, 255);
    fill_rect(0, SCREEN_H - 92, SCREEN_W, 92, 10, 2, 2, 170);
    if ((int)(t * 2) % 2 == 0 || t < 0.6f) draw_rich(&A.font, "PRESS [+] TO START", SCREEN_W / 2, SCREEN_H - 74, 0.7f, 1, 255, 236, 190, 255);
    draw_rich(&A.small, "(C) 2026 EGO STUDIOS", SCREEN_W - 20, SCREEN_H - 26, 0.36f, 2, 200, 180, 150, 180);
}

/* ---- save slots ---- */
static void open_slots(void) {
    for (int i = 0; i < SAVE_SLOTS; i++) slot_has[i] = save_peek(i, &slot_peek[i]);
    slot_sel = save_slot; slot_erase_armed = 0; scene = SC_SLOTS; scene_t = 0;
}
static void step_slots(unsigned anyp) {
    if (anyp & (B_LEFT | B_UP)) { slot_sel = (slot_sel + SAVE_SLOTS - 1) % SAVE_SLOTS; slot_erase_armed = 0; sfx("menu"); }
    if (anyp & (B_RIGHT | B_DOWN)) { slot_sel = (slot_sel + 1) % SAVE_SLOTS; slot_erase_armed = 0; sfx("menu"); }
    if (anyp & BACK_BITS()) { sfx("menu"); to_press(); return; }
    if ((anyp & B_LIGHTS) && slot_has[slot_sel]) {   /* X: erase (twice to be sure) */
        if (!slot_erase_armed) { slot_erase_armed = 1; sfx("tick"); }
        else { save_erase_slot(slot_sel); slot_has[slot_sel] = 0; slot_erase_armed = 0; sfx("boom"); message("SLOT ERASED", 1.6f); }
        return;
    }
    if (anyp & OK_BITS()) {
        sfx("confirm"); save_use_slot(slot_sel);
        p1_char = getenv("MM_CHAR") ? atoi(getenv("MM_CHAR")) % NCHARS : save.runner % NCHARS;
        save_write();   /* claims the slot, and tells us at once if the SD card is locked */
        scene = SC_TITLE; scene_t = 0; menu_sel = 1;
        if (save_write_failed()) message("THE SD CARD IS LOCKED OR MISSING - PROGRESS WON'T BE SAVED", 4.0f);
    }
}
static void draw_slots(void) {
    draw_curtains(1); draw_header("CHOOSE A SAVE SLOT");
    for (int i = 0; i < SAVE_SLOTS; i++) {
        float w = 340, h = 400, x = 80 + i * (w + 50), y = 140; int on = i == slot_sel;
        fill_rect((int)x - 8, (int)y - 8, (int)w + 16, (int)h + 16, on ? 230 : 70, on ? 180 : 40, on ? 80 : 30, 255);
        fill_rect((int)x, (int)y, (int)w, (int)h, 30, 14, 10, 255);
        char b[96]; snprintf(b, sizeof b, "SLOT %d", i + 1);
        draw_text_col(&A.font, b, x + w / 2, y + 16, 0.7f, 1, 255, 226, 160, 255);
        if (!slot_has[i]) { draw_text_col(&A.font, "NEW GAME", x + w / 2, y + 170, 0.75f, 1, on ? 255 : 200, on ? 240 : 190, on ? 200 : 160, 255);
            draw_text_col(&A.small, "AN EMPTY SEAT", x + w / 2, y + 230, 0.5f, 1, 200, 170, 140, 200); continue; }
        SaveData *d = &slot_peek[i];
        Tex *hero = &A.hero[d->runner % NCHARS][0][0];
        if (hero->tex) { float hk = 230.0f / hero->h; draw_ellipse(&A.disc, x + 95, y + 312, 60, 10, 0, 0, 0, 120);
            draw_tex_tint(hero, x + 95 - hero->w * hk / 2, y + 312 - hero->h * hk, hero->w * hk, hero->h * hk, 0, OUTFIT_RGB[d->outfit][0], OUTFIT_RGB[d->outfit][1], OUTFIT_RGB[d->outfit][2], 255, 0); }
        snprintf(b, sizeof b, "%s", CHAR_NAME[d->runner % NCHARS]); draw_text_col(&A.small, b, x + 230, y + 90, 0.6f, 1, 255, 236, 190, 255);
        if (d->true_ending) snprintf(b, sizeof b, "TRUE ENDING!"); else if (d->seen_ending) snprintf(b, sizeof b, "2ND SHOW: %d/8", d->cont_level + 1);
        else snprintf(b, sizeof b, "SHOW %d / 8", d->cont_level + 1);
        draw_text_col(&A.small, b, x + 230, y + 130, 0.5f, 1, 240, 210, 170, 255);
        int reels = 0; for (int s = 0; s < 8; s++) reels += popcount3(d->reels[s]);
        snprintf(b, sizeof b, "REELS %d/24", reels); draw_text_col(&A.small, b, x + 230, y + 166, 0.46f, 1, 255, 210, 140, 255);
        int best = 0; for (int s = 0; s < 8; s++) if (d->grade[s] > best) best = d->grade[s];
        if (best) draw_grade(x + 230, y + 250, best, 0.7f, 255);
        snprintf(b, sizeof b, "%s", LEVELS[d->cont_level].title); draw_text_col(&A.small, b, x + w / 2, y + 330, 0.46f, 1, 230, 200, 160, 255);
        if (on && slot_erase_armed) draw_rich(&A.small, "PRESS [X] AGAIN TO ERASE", x + w / 2, y + 362, 0.42f, 1, 255, 110, 80, 255);
    }
    draw_hint("[DPAD] CHOOSE   [OK] PLAY   [X] ERASE   [BACK] BACK"); draw_msg();
}

/* ---- attract mode: the game plays itself if nobody touches the title ---- */
static void start_demo(void) {
    static const int SHOWS[4] = {0, 1, 2, 3};
    demo_on = 1; demo_t = 0; rush_on = 0; loop_no = 0; mode = 0; paused = 0; demo_prev = 0;
    queue_show(SHOWS[demo_show++ % 4], 0);
}
static void end_demo(void) { demo_on = 0; paused = 0; audio_duck(0); to_press(); }
static Input demo_input(void) {   /* run right, shoot, hop now and then, back up a little sometimes */
    unsigned h = B_SHOOT | B_RIGHT; int f = (int)(demo_t * 60);
    if (f % 50 < 22) h |= B_JUMP;
    if (f % 420 > 380) h = (h & ~B_RIGHT) | B_LEFT;
    if (f % 300 > 250) h |= B_UP;
    Input in = {h, h & ~demo_prev}; demo_prev = h; return in;
}

/* ---- boss entrances: each boss arrives its own way during the name card ---- */
#define ENTER_K 0.42f   /* the moment of impact (fraction of the 2.6 s intro) */
static float ease_bounce(float t) { t = clampf(t, 0, 1); if (t < 0.6f) return t * t / 0.36f; float u = (t - 0.8f) / 0.2f; return 1 - 0.12f * (1 - u * u); }
static void boss_entrance_update(void) {
    if (!boss.active || boss.state != BS_INTRO) return;
    float k = 1 - boss.timer / 2.6f;
    int L = level_no;
    if (L == L_ENCORE && frame_no % 4 == 0 && k < ENTER_K) ink_burst(boss.x + (frand() - 0.5f) * 200, FLOOR_Y - 10, 4, 220);
    if (L == L_LOST && frame_no % 5 == 0 && k < ENTER_K) part_k(PK_DEBRIS, boss.x + (frand() - 0.5f) * 160, FLOOR_Y - 8, (frand() - 0.5f) * 400, -300 - frand() * 300, 1.2f, 6, 1500, 110, 90, 60, 0);
    if (k >= ENTER_K && !boss.intro) {
        boss.intro = 1; shake = 18; rumble_all(0.8f, 300);
        switch (L) {
        case L_LOBBY: case L_PIZZA: case L_INK: sfx("slam"); ring(boss.x, FLOOR_Y - 10, 260, 255, 220, 160); smoke(boss.x, FLOOR_Y - 10, 8, 90); ink_burst(boss.x, FLOOR_Y - 10, 18, 380); break;
        case L_REELS: case L_MIRROR: sfx("shatter"); spark_burst(boss.x, boss.y - 200, 30, 255, 250, 230); negative_t = 0.25f; break;
        case L_ROOF: sfx("thunder"); bolt_add(boss.x, 0.05f); spark_burst(boss.x, boss.y - 200, 40, 255, 250, 200); break;
        default: sfx("splash"); ink_burst(boss.x, FLOOR_Y - 10, 30, 420); ring(boss.x, FLOOR_Y - 10, 300, 200, 160, 220); break;
        }
        sfx(L == L_PIZZA ? "angry" : "roar1");
    }
}
static void draw_boss_inner(float ox, float oy);
static void draw_boss(float ox, float oy) {
    if (!boss.active || boss.state != BS_INTRO) { draw_boss_inner(ox, oy); return; }
    float k = 1 - boss.timer / 2.6f, e = ease_bounce(k / ENTER_K), bx = boss.x, by = boss.y;
    switch (level_no) {
    case L_LOBBY: boss.y = by - 760 * (1 - e); break;                                         /* drops from the rafters */
    case L_PIZZA: boss.x = bx + 700 * (1 - e); boss.y = by - 320 * sinf(3.14159f * clampf(k / ENTER_K, 0, 1)); break;   /* leaps in */
    case L_INK: boss.x = bx + 900 * (1 - clampf(k / ENTER_K, 0, 1)); boss.y = by - fabsf(sinf(k * 40)) * 10 * (1 - e); break;   /* rolls in */
    case L_LOST: case L_ENCORE: boss.y = by + 520 * (1 - e); break;                            /* rises from below */
    case L_REELS: case L_MIRROR: if (k < ENTER_K && (frame_no / 3) % 2) return; if (k < ENTER_K) boss.x = bx + (frand() - 0.5f) * 40; break;   /* flickers in */
    case L_ROOF: if (k < ENTER_K) return; break;                                               /* revealed by lightning */
    }
    draw_boss_inner(ox, oy);
    boss.x = bx; boss.y = by;
}

/* ---- aftermath card ---- */
static void load_aftermath(int L) {
    tex_free(&aftermath_tex);
    if (rush_on) return;
    char rel[64]; snprintf(rel, sizeof rel, "ui/aftermath_%d.jpg", L); aftermath_tex = tex_load(rel);
}
static void draw_aftermath(void) {
    float t = scene_t, a = clampf(fminf(t / 0.4f, (AFTER_T - t) / 0.35f), 0, 1);
    fill_rect(0, 0, SCREEN_W, SCREEN_H, 0, 0, 0, (Uint8)(255 * clampf(t / 0.3f, 0, 1)));
    float z = 1.0f + 0.05f * t / AFTER_T, w = SCREEN_W * z, h = SCREEN_H * z;
    draw_tex(&aftermath_tex, SCREEN_W / 2 - w / 2, SCREEN_H / 2 - h / 2, w, h, 0, (Uint8)(255 * a));
    fill_rect(0, SCREEN_H - 96, SCREEN_W, 96, 8, 3, 2, (Uint8)(200 * a));
    draw_text_col(&A.font, AFTER_LINE[level_no], SCREEN_W / 2, SCREEN_H - 78, 0.62f, 1, 255, 236, 190, (Uint8)(255 * a));
}

/* ---- voices and surfaces ---- */
static void vo(int chr, const char *line) { char b[32]; snprintf(b, sizeof b, "vo_%s_%s", chr == 1 ? "rosie" : "joe", line); sfx(b); }
static const char *land_sfx(void) { return level_no == L_PIZZA ? "land_tile" : (level_no == L_INK || level_no == L_ROOF) ? "land_metal" : "land_wood"; }

/* ==================================================================================================
   STAGEHAND BADGES (achievements)
   ================================================================================================== */
static const char *const BADGE[20][2] = {
    {"FIRST NIGHT JITTERS", "BEAT SHOW 1"},
    {"HALFWAY THERE", "BEAT SHOW 4"},
    {"THE FINAL BOW", "BEAT SHOW 8"},
    {"TRUE BELIEVER", "SEE THE TRUE ENDING"},
    {"REEL FIND", "FIND YOUR FIRST FILM REEL"},
    {"FILM BUFF", "FIND 12 FILM REELS"},
    {"THE WHOLE COLLECTION", "FIND ALL 24 FILM REELS"},
    {"STAR OF THE SHOW", "EARN AN S RANK"},
    {"STANDING OVATION", "EARN S RANKS ON ALL 8 SHOWS"},
    {"HARD HAT HERO", "BEAT A SHOW WITHOUT GETTING HIT"},
    {"UNTOUCHABLE", "BEAT A BOSS WITHOUT GETTING HIT"},
    {"QUICK CHANGE", "BEAT A SHOW IN UNDER 4 MINUTES"},
    {"ELECTRICIAN OF THE YEAR", "FLIP 25 BREAKERS"},
    {"MIDNIGHT RUN", "BEAT A SHOW ON MIDNIGHT (HARD)"},
    {"ENCORE! ENCORE!", "FINISH THE BOSS RUSH"},
    {"RUSH HOUR", "FINISH THE BOSS RUSH IN UNDER 8 MINUTES"},
    {"BUDDY SYSTEM", "BEAT A SHOW WITH TWO PLAYERS"},
    {"EYES ON THE CAMERAS", "BEAT A SHOW IN SECURITY CO-OP"},
    {"DRESSED FOR SUCCESS", "WEAR A NEW OUTFIT"},
    {"HIDE AND SEEK", "HIDE IN A PROP CLOSET"},
};
static int badge_sel; static float toast_t; static int toast_id = -1;
static int has_badge(int b) { return (save.badge[b / 8] >> (b % 8)) & 1; }
static void award(int b) {
    if (demo_on || b < 0 || b >= 20 || has_badge(b)) return;
    save.badge[b / 8] |= (unsigned char)(1 << (b % 8)); game_save();
    toast_id = b; toast_t = 3.6f; sfx("reel");
}
static void draw_medal(float x, float y, float r, int got) {
    if (got) draw_glow(&A.disc, x, y, r * 2, 255, 200, 90, 150);
    fill_rect((int)(x - r * 0.35f), (int)(y - r * 1.9f), (int)(r * 0.3f), (int)(r * 1.2f), got ? 190 : 60, got ? 40 : 30, got ? 40 : 30, 255);
    fill_rect((int)(x + r * 0.05f), (int)(y - r * 1.9f), (int)(r * 0.3f), (int)(r * 1.2f), got ? 40 : 30, got ? 70 : 30, got ? 160 : 40, 255);
    draw_ellipse(&A.disc, x, y, r, r, got ? 120 : 40, got ? 80 : 32, got ? 20 : 28, 255);
    draw_ellipse(&A.disc, x, y, r * 0.84f, r * 0.84f, got ? 255 : 70, got ? 205 : 60, got ? 80 : 50, 255);
    if (got) { draw_bar(x - r * 0.45f, y, x + r * 0.45f, y, r * 0.18f, 160, 110, 30, 255); draw_bar(x, y - r * 0.45f, x, y + r * 0.45f, r * 0.18f, 160, 110, 30, 255); }
    else draw_text_col(&A.small, "?", x, y - r * 0.6f, r / 30.0f, 1, 120, 100, 80, 255);
}
static void step_badges(unsigned anyp) {
    if (anyp & B_LEFT) { badge_sel = (badge_sel + 19) % 20; sfx("menu"); }
    if (anyp & B_RIGHT) { badge_sel = (badge_sel + 1) % 20; sfx("menu"); }
    if (anyp & B_UP) { badge_sel = (badge_sel + 15) % 20; sfx("menu"); }
    if (anyp & B_DOWN) { badge_sel = (badge_sel + 5) % 20; sfx("menu"); }
    if (anyp & (BACK_BITS() | OK_BITS())) { sfx("menu"); scene = SC_EXTRAS; scene_t = 0; }
}
static void draw_badges(void) {
    draw_curtains(1); draw_header("STAGEHAND BADGES");
    for (int i = 0; i < 20; i++) { float x = 70 + (i % 4) * 290, y = 150 + (i / 4) * 80; int on = i == badge_sel, got = has_badge(i);
        if (on) fill_rect((int)x - 8, (int)y - 6, 284, 70, 120, 26, 22, 200);
        draw_medal(x + 32, y + 36, 20, got);
        const char *nm = got ? BADGE[i][0] : "? ? ?";
        draw_text_col(&A.small, nm, x + 64, y + 20, fit(&A.small, nm, 0.4f, 206), 0, got ? 255 : 160, got ? 230 : 140, got ? 170 : 120, 255); }
    char b[128]; snprintf(b, sizeof b, "%s", has_badge(badge_sel) ? BADGE[badge_sel][0] : "? ? ?");
    draw_text_col(&A.font, b, SCREEN_W / 2, 562, fit(&A.font, b, 0.55f, 1100), 1, 255, 226, 160, 255);
    draw_text_col(&A.small, BADGE[badge_sel][1], SCREEN_W / 2, 610, fit(&A.small, BADGE[badge_sel][1], 0.5f, 1100), 1, 230, 200, 160, 255);
    draw_hint("[DPAD] CHOOSE   [BACK] BACK");
}
static void draw_toast(void) {
    if (toast_t <= 0 || toast_id < 0) return;
    toast_t -= DT * (scene == SC_PLAY && paused ? 0 : 1) ;
    float k = fminf(1, fminf((3.6f - toast_t) * 4, toast_t * 3)), x = SCREEN_W - 430 + (1 - k) * 460, y = 96;
    fill_rect((int)x, (int)y, 410, 76, 30, 12, 8, 235); fill_rect((int)x, (int)y, 410, 3, 230, 180, 80, 255);
    draw_medal(x + 42, y + 42, 22, 1);
    draw_text_col(&A.small, "BADGE EARNED", x + 84, y + 10, 0.42f, 0, 255, 200, 110, 255);
    draw_text_col(&A.small, BADGE[toast_id][0], x + 84, y + 38, fit(&A.small, BADGE[toast_id][0], 0.5f, 316), 0, 255, 240, 210, 255);
}
