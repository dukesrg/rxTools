# Copyright (C) 2015 The PASTA Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

ifeq ($(strip $(RXTOOLSMK)),)
$(error "Please run `make` at the top directory")
endif

.PHONY: all clean
sp :=
sp +=
THEMES := $(subst .json+,.json ,$(subst $(sp),+,$(wildcard *.json)))
IMAGES := $(subst .png+,.png ,$(foreach d,$(THEMES),$(subst $(sp),+,$(wildcard $(subst +,\ ,$(basename $d)/*.png)))))

all: $(IMAGES:.png=.bgr)

%.bgr:
	$(call Q,MAGICK,$@)magick $(subst +,\ ,$(addsuffix .png,$(basename $@))) -rotate 90 $(subst +,\ ,$@);

clean: $(IMAGES:.png=.rmbgr)

%.rmbgr:
	@rm -f $(subst +,\ ,$(addsuffix .bgr,$(basename $@)))