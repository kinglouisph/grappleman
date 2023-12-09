//menu
float menuVerts[8] = {
  //Start
  -.7, .1,
  .7, .1,
  -.7, -.1,
  .7, -.1 
};

char menuInds[6] = {
  0,1,2,
  1,2,3
};

float swarmVerts[6] = {
  .05, 0,
  -.05, .02,
  -.05, -.02
};

#define playerWeaponWidth 0.04f
#define playerWeaponLength 0.1f
#define playerWeaponOffset 0.01f


float playerWeapon[6] = {
  playerWeaponOffset, playerWeaponWidth,
  playerWeaponOffset, playerWeaponWidth,
  playerWeaponOffset + playerWeaponLength, 0.0f
};

float bulletVerts[6] = {
  0,0,
  -0.04,-0.01,
  -0.04,0.01
};

float powerUpVerts[14] = {
  0,.2,
  -.1, .1,
  .1, .1,
  .04, .1,
  -.04,.1,
  .04,-.18,
  -.04,-.18
};

char powerUpInds[9] = {
  0,1,2,
  3,4,5,
  4,5,6
};