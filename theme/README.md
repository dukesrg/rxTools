# New theme formet description will be here (sidenotes while WIP)

Menu attribute values are inherited from parents and applied on displayed menu to all contained subitems:

```JSON
{
  "name":"rxTools" theme name, will be displayed in interface (planned)
  "menu":{ top menu hierarchy tag, binds by name to gui.json
    "color":"ffffffff", menu item text color (or pair), backgorund is transparent if second color is omitted (default - white on transparent)
    "selcolor":["ff000000","ffffffff"], selected menu item color pair (default - black on white)
    "disabled":["ff7f7f7f"], not available menu item text color (default - grey on transparent)
    "unselected":["ff7f7f7f","ffffffff"], selected not available menu item color pair (default - grey on white)
    "hint":"ff008dc5", hint color (default - white on transparent)
    "value":"ff008dc5", current value color, e.x. settings values (default - white on transparent)
    "topimg":["top.bin","top.bin"], images for top screen, if second is omitted, used one for both (i.e. no 3D, planned)
    "bottomimg":"base.bin", image for bottom screen
      "ITEM1":{ subitems from gui.json, binds by name to gui.json, containing any of the above attributes.
        "ITEM1-1":{
        ..
        }
      }
    }
  }
}    
```