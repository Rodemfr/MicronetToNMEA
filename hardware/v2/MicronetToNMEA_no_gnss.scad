$fs = 0.1;

// Case
ctn = 2;  // Case thickness
ch = 30;  // Case height
fs = 3.5; // Free space between PCB & case

// PCB
pcbw = 51.539; // PCB width
pcbl = 56.644; // PCB length
pcbt = 1.6;    // PCB thickness

// Board pins
ph = 5;       // Height
pr = 2;       // Radius
pshr = 0.9;   // Screw hole radius
fpx = 3.048;  // First pin x position from west PCB border
fpy = 2.921;  // First pin y position from south PCB border
pxd = 45.720; // x distance between pins
pyd = 34.290; // y distance between pins

// Screw terminals
pstx = 29.969;  // x position from west PCB border (+12V)
aisty = 17.497; // y position from south PCB border (AIS)
sth = 8;        // Height
stw = 10;       // Width

// Screw pads
spw = 10; // Pad width
sdia = 2; // screw diameter

// Teensy USB connector
tuch = 8;
tucw = 11;
tucx = 11.808 + 15.24 / 2;
tucz = 13.5;

// 868MHz Antenna
antw = 6;  // Hole diameter
antz = 14; // z position of the antenna hole from PCB surface

// Bottom panel
cube([ pcbw + fs * 2, pcbl + fs * 2, ctn ]);

// South panel
difference()
{
    // Panel
    cube([ pcbw + fs * 2, ctn, ch ]);
    // Power supply connector hole
    translate([ pstx + fs, -0.1, ctn + ph + pcbt ]) cube([ stw, ctn + 1, sth ]);
    // Teensy USB connector hole
    translate([ tucx + fs, -0.1, ctn + ph + pcbt + tucz ]) cube([ tucw, ctn + 3, tuch ], center = true);
}

// North panel
translate([ 0, pcbl + fs * 2 - ctn, 0 ]) cube([ pcbw + fs * 2, ctn, ch ]);

// West panel
cube([ ctn, pcbl + fs * 2, ch ]);

// East panel
difference()
{
    // Panel
    translate([ pcbw + fs * 2 - ctn, 0, 0 ]) cube([ ctn, pcbl + fs * 2, ch ]);
    // AIS connector hole
    translate([ pcbw + fs * 2 - ctn - 0.1, aisty + fs, ctn + ph + pcbt ]) cube([ 5, stw, sth ]);
    // 868MHz Antenna hole
    translate([ pcbw + fs * 2 - ctn - 0.1, 45 + fs, ctn + ph + pcbt + antz ]) rotate([ 0, 90, 0 ])
        cylinder(10, antw / 2, antw / 2);
}

// Board pin definition
module boardPin(x, y)
{
    difference()
    {
        // Pin
        translate([ x, y, 0 ]) cylinder(ph + ctn, pr, pr);
        // Screw hole
        translate([ x, y, 0 ]) cylinder(ph + ctn + 0.1, pshr, pshr);
    }
}
// PCB pins
boardPin(fpx + fs, fpy + fs);
boardPin(fpx + fs, fpy + fs + pyd);
boardPin(fpx + fs + pxd, fpy + fs + pyd);

module screwPad()
{
    difference()
    {
        cube([ spw, spw, ctn ]);
        translate([ spw / 2, spw / 2, -1 ]) cylinder(ctn + 2, sdia / 2 + 0.1, sdia / 2 + 0.1);
    }
}

translate([ -spw + 0.001, 0, 0 ]) screwPad();
translate([ -spw + 0.001, pcbl + fs * 2 - spw, 0 ]) screwPad();
translate([ pcbw + fs * 2 - 0.001, 0, 0 ]) screwPad();
translate([ pcbw + fs * 2 - 0.001, pcbl + fs * 2 - spw, 0 ]) screwPad();

// Top panel
if (1)
{
    panelShift = 15;
    translate([ pcbw + fs * 2 + panelShift, 0, 1 ]) cube([ pcbw + fs * 2, pcbl + fs * 2, ctn ]);
    translate([ pcbw + fs * 2 + panelShift + ctn, ctn, 0 ])
        cube([ pcbw + fs * 2 - ctn * 2, pcbl + fs * 2 - ctn * 2, 1.1 ]);

    // MicronetToNMEA Logo
    translate([ pcbw + fs * 2 + panelShift + ctn, pcbl * 0.45 + ctn, 3.2 ])
        text("MicronetToNMEA", size = 4.2, font = "Sans:style=Bold");
}