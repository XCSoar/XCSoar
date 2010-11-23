import sys
import os
import convert_lkm

def main():
    if len(sys.argv) < 2:
        print "Too few arguments given! Please provide a working folder."
        return
    
    folder = sys.argv[1]
    if not os.path.exists(folder) and os.path.isdir(folder):
        print "Working folder \"" + folder + "\" does not exist!"
        return
    
    lkm_files = []
    xcm_files = []
    for file in os.listdir(folder):
        if file.lower().endswith(".lkm"):
            lkm_files.append(file[:-4])
        if file.lower().endswith(".xcm"):
            xcm_files.append(file[:-4])

    # Filter already created XCM files from the todo list
    for file in xcm_files:
        try: lkm_files.remove(file)
        except ValueError: pass
      
    for file in lkm_files:
        template = convert_lkm.read_template(os.path.join(folder, file + ".LKM"))
        convert_lkm.convert(template, folder)
        print "-------------------"
    
if __name__ == '__main__':
    main()    
