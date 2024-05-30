#pragma once

#ifndef fonthandler_h
#define fonthandler_h

#include <filesystem>
#include <algorithm>
#include <vector>
#include <memory>
#include <map>
#include <string>




#include "blend2d/blend2d.h"

#include "maths.h"
#include "bspan.h"




namespace waavs {
    class FontHandler
    {
    public:
        // Typography
        BLFontManager fFontManager{};
        std::vector<std::string> fFamilyNames{};

        // For size helper
        int fDotsPerInch = 1;
        float fUnitsPerInch = 1;

        FontHandler()
        {
            fFontManager.create();
        }

        /*
            setDpiUnits makes it possible to let the FontHandler to
            understand the DPI we're rendering to, as well as
            the intended units we're using to specify font sizes.

            This combination allows you to say; "my display is 192 dpi,
            and I want to use 'inches' to specify my font sizes"

            setDpiUnits(192, 1);

            Or, if you want to work in millimeters

            setDpiUnits(192, 25.4);

            If you don't know your displays dpi, and you just
            want to work in pixels, then you can use the default
            which is;

            setDpiUnits(1,1);

        // Some typical units
            local unitFactors = {
            ["in"] = 1;
            ["mm"] = 25.4;
            ["px"] = 96;
            ["pt"] = 72;
        }
        */
        void setDpiUnits(int dpi, float unitsPerInch)
        {
            fDotsPerInch = dpi;
            fUnitsPerInch = unitsPerInch;
        }

        const std::vector<std::string>& familyNames() const { return fFamilyNames; }




        // create a single font face by filename
        // Put it into the font manager
        // return it to the user
        BLFontFace loadFontFace(const char* filename)
        {
            BLFontFace ff;
            //BLResult err = ff.createFromFile(filename, BL_FILE_READ_MMAP_AVOID_SMALL);
            BLResult err = ff.createFromFile(filename);

            if (!err)
            {
                //printf("loadFont() adding: %s\n", ff.familyName().data());
                //BLBitSet aSet;
				//blBitSetInit(&aSet);

				//ff.getCharacterCoverage(&aSet);
				//printf("FontHandler::loadFont() coverage: %d\n", aSet.cardinality());
                
                fFontManager.addFace(ff);
                fFamilyNames.push_back(std::string(ff.familyName().data()));
            }
            else {
                ;
                printf("FontHandler::loadFont Error: %s (0x%x)\n", filename, err);
            }

            return ff;
        }

        // Load the list of font files into 
        // the font manager
        void loadFonts(std::vector<const char*> fontNames)
        {
            for (const auto& filename : fontNames)
            {
                BLFontFace face = loadFontFace(filename);
                //printf("loadFonts: %s, 0x%x\n", face.familyName().data(), face.isValid());
            }
        }

        void loadDirectoryOfFonts(const char* dir)
        {
            const std::filesystem::path fontPath(dir);

            for (const auto& dir_entry : std::filesystem::directory_iterator(fontPath))
            {
                if (dir_entry.is_regular_file())
                {
                    if (dir_entry.path().generic_string().ends_with(".ttf") || 
                        dir_entry.path().generic_string().ends_with(".TTF") ||
                        dir_entry.path().generic_string().ends_with(".otf"))
                    {
                        BLFontFace ff;
                        ff = loadFontFace(dir_entry.path().generic_string().c_str());
                    }
                }
            }
        }

        // Do this only once to load in fonts
        void loadDefaultFonts()
        {
            // Load in some fonts to start
            std::vector<const char*> fontNames{
                "c:\\Windows\\Fonts\\arial.ttf",
                "c:\\Windows\\Fonts\\calibri.ttf",
                "c:\\Windows\\Fonts\\cascadiacode.ttf",
                "c:\\Windows\\Fonts\\consola.ttf",
                "c:\\Windows\\Fonts\\cour.ttf",
                "c:\\Windows\\Fonts\\gothic.ttf",
                "c:\\Windows\\Fonts\\segoui.ttf",
                "c:\\Windows\\Fonts\\tahoma.ttf",
                "c:\\Windows\\Fonts\\terminal.ttf",
                "c:\\Windows\\Fonts\\times.ttf",
                "c:\\Windows\\Fonts\\verdana.ttf",
                "c:\\Windows\\Fonts\\wingding.ttf"
            };
            loadFonts(fontNames);
        }

        float getAdjustedFontSize(float sz) const
        {
            float fsize = sz * ((float)fDotsPerInch / fUnitsPerInch);
            return fsize;
        }
        
        
        // selectFontFamily
        // Select a specific family, where a list of possibilities have been supplied
        // The query properties of style, weight, and stretch can also be supplied
        // with defaults of 'normal'
		bool selectFontFamily(const ByteSpan& names, BLFontFace& face, uint32_t style= BL_FONT_STYLE_NORMAL, uint32_t weight= BL_FONT_WEIGHT_NORMAL, uint32_t stretch= BL_FONT_STRETCH_NORMAL)
		{
            charset delims(",");
            charset quoteChars("'\"");

            ByteSpan s = names;
            
            BLFontQueryProperties qprops{};
            qprops.stretch = stretch;
            qprops.style = style;
            qprops.weight = weight;
            
            // nammes are quoted, or separated by commas
            while (s.size() > 0)
            {
                s = chunk_ltrim(s, wspChars);
                ByteSpan name = chunk_token(s, delims);
                name = chunk_trim(name, quoteChars);

                if (!name)
                    break;

                BLStringView familyNameView{};
                bool success{ false };

                if ((name == "Sans") ||
                    (name == "sans") ||
                    (name == "sans-serif")) {
                    success = (BL_SUCCESS == fFontManager.queryFace("Arial", qprops, face));
                }
                else {
                    familyNameView.reset((char*)name.fStart, name.size());
                    success = (BL_SUCCESS == fFontManager.queryFace(familyNameView, qprops, face));
                }

				if (success)
					return true;

				// Didn't find it, try the next one
                printf("== FontHandler::selectFontFamily, NOT FOUND: ");
                printChunk(name);
			}
            
			return false;
		}
        
        
        // Select a font with given criteria
		// If the font is not found, then return the default font
        // which should be Arial
        bool selectFont(const ByteSpan& names, BLFont& font, float sz, uint32_t style = BL_FONT_STYLE_NORMAL, uint32_t weight = BL_FONT_WEIGHT_NORMAL, uint32_t stretch = BL_FONT_STRETCH_NORMAL)
        {
            BLFontFace face;

            // Try to select an appropriate font family
            bool success = selectFontFamily(names, face, style, weight, stretch);
            
            // If none found, try our default font
            if (!success)
            {
                success = selectFontFamily("Arial", face, style, weight, stretch);
                
                if (!success)
                    return false;
            }
            
            // Now that we've gotten a face, we need to fill in the font
            // object to be the size we want
            float fsize = getAdjustedFontSize(sz);
			blFontReset(&font);
            
            success = (BL_SUCCESS == blFontCreateFromFace(&font, &face, fsize));
            
			return success;
        }
        

        
        // This is fairly expensive, and should live with a font object
        // instead of on this interface
        vec2f textMeasure(const ByteSpan & txt, const char* familyname, float sz)
        {
            BLFont afont{};
            auto success = selectFont(familyname, afont, sz);
            
			if (!success)
				return vec2f(0, 0);
            
            BLTextMetrics tm{};
            BLGlyphBuffer gb;
            gb.setUtf8Text(txt.fStart, txt.size());
            afont.shape(gb);
            afont.getTextMetrics(gb, tm);

            float cx = (float)(tm.boundingBox.x1 - tm.boundingBox.x0);
            float cy = afont.size();

            return { cx, cy };
        }
    };

}

#endif // fonthandler_h