import sys
import os
import convert_lkm

def main():
    if len(sys.argv) < 2:
        print "Too few arguments given! Please provide a template folder."
        return
    
    folder = sys.argv[1]
    if not os.path.exists(folder) and os.path.isdir(folder):
        print "Template folder \"" + file + "\" does not exist!"
        return
    
    for file in os.listdir(folder):
        if file.endswith(".TXT"):
            template = convert_lkm.read_template(os.path.join(folder, file))
            convert_lkm.convert(template, folder)
    
if __name__ == '__main__':
    main()    
