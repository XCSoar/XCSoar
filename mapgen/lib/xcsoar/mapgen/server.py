import os
import shutil
import cherrypy
from genshi.filters import HTMLFormFiller
from xcsoar.mapgen.job import Job
from xcsoar.mapgen import view
from xcsoar.mapgen.georect import GeoRect

class Server(object):
    def __init__(self, dir_jobs):
        self.__dir_jobs = os.path.abspath(dir_jobs)

    @cherrypy.expose
    @view.output('index.html')
    def index(self):
        return view.render()

    @cherrypy.expose
    @view.output('index.html')
    def generate(self, name, mail, waypoint_file, latmin, latmax, lonmin, lonmax):
        name = name.strip()
        if name == "":
            return view.render(error='No map name given!') | HTMLFormFiller(data=dict(name=name, mail=mail))

        if not waypoint_file.file:
            return view.render(error='Waypoint file could not be read!') | HTMLFormFiller(data=dict(name=name, mail=mail))

        job = Job(self.__dir_jobs)
        job.description.name = name
        job.description.mail = mail
        job.description.waypoint_file = 'waypoints.dat'

        try:
            job.description.bounds = GeoRect(float(lonmin.strip()), float(lonmax.strip()), float(latmax.strip()), float(latmin.strip()))
        except:
            pass

        path = job.file_path(job.description.waypoint_file)
        f = open(path, "w")
        shutil.copyfileobj(fsrc=waypoint_file.file, fdst=f, length=1024 * 64)
        f.close()

        if os.path.getsize(path) == 0:
            os.unlink(path)
            job.description.waypoint_file = None

        if job.description.bounds == None and job.description.waypoint_file == None:
            job.delete()
            return view.render(error='Waypoint file or bounds are required!') | HTMLFormFiller(data=dict(name=name, mail=mail))

        job.enqueue()
        raise cherrypy.HTTPRedirect('/status?uuid=' + job.uuid)

    @cherrypy.expose
    @view.output('status.html')
    def status(self, uuid):
        job = Job.find(self.__dir_jobs, uuid)
        if job == None:
            return view.render('error.html', error='Job not found!')
        status = job.status()
        if status == 'Error':
            return view.render('error.html', error='Generation failed!')
        elif status == 'Done':
            return view.render('done.html', name=job.description.name, uuid=uuid)
        return view.render(uuid=uuid, name=job.description.name, status=status)

    @cherrypy.expose
    def download(self, uuid):
        job = Job.find(self.__dir_jobs, uuid)
        if not job or job.status() != 'Done':
            return self.status(uuid)
        return cherrypy.lib.static.serve_download(job.map_file(), job.description.name + '.xcm')
