import os
import cherrypy
from map_daemon import MapJob
import pickle
import hashlib
import random

dir_jobs = "../jobs"

class MapServer(object):
    def generate_uuid(self):
        m = hashlib.sha1()
        m.update(str(random.random()))
        return m.hexdigest()
    
    def get_dir_job(self, uuid):
        return os.path.join(dir_jobs, uuid)
    
    def dir_job_exists(self, uuid):
        return os.path.exists(self.get_dir_job(uuid))
    
    def get_file_status(self, uuid):
        return os.path.join(self.get_dir_job(uuid), "status")
    
    def get_file_job(self, uuid):
        return os.path.join(self.get_dir_job(uuid), "job")
    
    def get_job_lock(self, uuid):
        return os.path.join(self.get_dir_job(uuid), "job.lock")
    
    def get_download_lock(self, uuid):
        return os.path.join(self.get_dir_job(uuid), "download.lock")
    
    def create_dir_job(self, uuid):
        dir_job = self.get_dir_job(uuid)
        if not os.path.exists(dir_job):
            os.mkdir(dir_job)
        return dir_job
        
    def lock_job(self, uuid):
        open(self.get_job_lock(uuid), "w").close()
    
    def unlock_job(self, uuid):
        job_lock = self.get_job_lock(uuid)
        if os.path.exists(job_lock):
            os.unlink(job_lock)
            
    def get_job_status(self, uuid):
        if not self.dir_job_exists(uuid):
            return None
        
        file_status = self.get_file_status(uuid)
        if not os.path.exists(file_status):
            return "Queued ..."
        
        f = open(file_status)
        ret = f.readline()
        f.close()
        
        return ret
    
    def header(self):
        return """<html>
<head>
    <title>XCSoar - Map Server</title>
    <link rel="stylesheet" type="text/css" href="/style" /> 
</head>
<body>
"""

    def footer(self):
        return """
</body>
</html>
"""
        
    def surround(self, html):
        return self.header() + html + self.footer()
    
    @cherrypy.expose
    def style(self):
        return """body { margin: 0px; font-family: "Lucida Grande", "Lucida Sans Unicode", "Lucida Sans", Helvetica, Arial, sans-serif; }
form p, div { margin-bottom: 20px; padding-right: 20px; font-family: Georgia, serif; font-size: 22px; line-height: 40px; font-weight: normal; }
form div.box { width: 580px; padding: 10px; margin-bottom: 4px; border: 1px solid #9f9c99; border-radius: 8px; font-family: "Lucida Grande", "Lucida Sans Unicode", "Lucida Sans", Helvetica, Arial, sans-serif;  font-size: 20px; }
form input.text { width: 100%; border: 0px; font-family: "Lucida Grande", "Lucida Sans Unicode", "Lucida Sans", Helvetica, Arial, sans-serif;  font-size: 20px; }
form input.button { width: 200px; cursor: pointer; padding: 8px 20px; margin-left: 205px; font-size: 18px; text-transform: uppercase; border: 0; border-radius: 8px; cursor: pointer; }
form input.button:hover, form input.button:focus { background: #1B8D29; color: #fff; }
"""

    @cherrypy.expose
    def index(self):
        html = """<form action="/generate/" method="post" enctype="multipart/form-data">
    <div>
        Map name:<br/>
        <div class="box">
            <input type="text" name="name" class="text" />
        </div>
    </div>
    <div>
        eMail:<br/>
        <div class="box">
            <input type="text" name="mail1" class="text" /><br/>
        </div>
        <div class="box">
            <input type="text" name="mail2" class="text" />
        </div>
    </div>
    <div>
        Waypoint file:<br/>
        <div class="box">
            <input type="file" name="waypoint_file" />
        </div>
    </div>
    <div>
        <input type="submit" value="Generate" class="button" id="submit">
    </div>
</form>"""
        return self.surround(html)
    
    def error(self, error):
        return self.surround(error)
    
    @cherrypy.expose
    def generate(self, name, mail1, mail2, waypoint_file):
        name = name.strip()
        if name == "":
            return self.error("No map name given!")
        
        mail1 = mail1.strip()
        mail2 = mail2.strip()
        if mail1 != mail2:
            return self.error("eMail addresses don't match!")
        
        if mail1 == "" or mail2 == "":
            return self.error("No eMail address given!")
        
        if not waypoint_file.file or not os.path.exists(waypoint_file.file.name):
            return self.error("Waypoint file could not be read!")
            
        uuid = self.generate_uuid()

        dir_job = self.create_dir_job(uuid)
        self.lock_job(uuid)
        
        path = os.path.join(dir_job, "waypoints.dat")
        f = open(path, "w")
        while True:
            data = waypoint_file.file.read(8192)
            if not data:
                break
            f.write(data)
        f.close()

        file_job = self.get_file_job(uuid)
        
        job = MapJob()
        job.name = name
        job.mail = mail1
        job.command = "generate"
        job.use_waypoint_file = True
           
        f = open(file_job, "wb")
        pickle.dump(job, f)
        f.close()
       
        self.unlock_job(uuid)
       
        return self.status(uuid)
 
    @cherrypy.expose
    def status(self, uuid):
        status = self.get_job_status(uuid)
        if status == None:
            return self.error("Job not found!")
        
        if status == "Done":
            return self.surround("Map ready for <a href=\"/download?uuid=" +
                                 uuid + "\">Download</a>")
        
        return self.surround(status + "<br/>" + 
                             "<a href=\"/status?uuid=" + uuid + 
                             "\">Refresh</a>" + 
"""
<script>
setTimeout(function(){ location.href = '/status?uuid=""" + uuid + """'; }, 10000);
</script>
""")
    
    @cherrypy.expose
    def download(self, uuid):
        if self.get_job_status(uuid) != "Done":
            return self.status(uuid)

        dir_job = self.get_dir_job(uuid)
        download_lock = self.get_download_lock(uuid)

        f = open(download_lock, "r")
        job = pickle.load(f)
        f.close()

        file_map = os.path.abspath(os.path.join(dir_job, "map.xcm"))
        return cherrypy.lib.static.serve_download(file_map, job.name + ".xcm")
        
if __name__ == '__main__':
    cherrypy.config.update({'server.socket_port': 8037})
    cherrypy.quickstart(MapServer())