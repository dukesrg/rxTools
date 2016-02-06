# New theme formet description will be here (sidenotes while WIP)

Menu attributes, values propagated to nested menus (i.e. if not explicitly set, the value from the upper level will be used)

```JSON
{
  "name":"rxTools", theme name in the top object, menu names in nested objects
  "color":"ffffffff", menu item text color, backgorund is transparent if second color is omitted (default - white on transparent)
  "selcolor":["ff000000","ffffffff"], selected menu item color pair (default - black on white)
  "nacolor":["ff7f7f7f"], not available menu item text color (default - grey on transparent)
  "selnacolor":["ff7f7f7f","ffffffff"], selected not available menu item color pair (default - grey on white)
  "imgtop":["imgtopl.bgr","imgtopr.bgr"], images for top screen, if second is omitted, used one for both (i.e. no 3D)
  "imgbottom":"img2.bgr", image for bottom screen
  "menu": menu
  [
    {
      "name":"Main menu item1",
      ... menu attributes override
      "menu": submenu level 1
      [
        {
          "name":"Sub menu item1",
          ... submenu attributes override
        },
        {
          ...
        }
      ]
    },
    {
      ...
    }
  ]
}
```
