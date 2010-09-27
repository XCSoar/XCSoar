import os
import time

from map_generator import MapGenerator
import pickle

class MapJob:
    output_file = None
    waypoint_file = None
    waypoint_details_file = None
    airspace_file = None
    bounds = None
    resolution = 9.0
    no_topology = False
    no_terrain = False
    
    def __init__(self, command = None):
        self.command = command

class MapDaemon:
    def __init__(self, dir_jobs = "../jobs/"):
        self.__dir_jobs = os.path.abspath(dir_jobs)
        self.__run = False
    
    def __lock_download(self, dir_job):
        open(os.path.join(dir_job, "download.lock"), "w").close()
    
    def __delete_job(self, file_job, complete = True):
        dir_job = os.path.dirname(file_job)
        if complete:
            for file in os.listdir(dir_job):
                os.unlink(os.path.join(dir_job, file))
            os.rmdir(dir_job)
        else:
            if os.path.exists(file_job):
                os.unlink(file_job)
            
        print "Job deleted (" + file_job + ")"
        
    def __read_job(self, file_job):
        if not os.path.exists(file_job):
            return None
        
        f = open(file_job, "r")
        job = pickle.load(f)
        f.close()
        
        job.file_job = file_job
        
        return job
    
    def __update_job_status(self, dir_job, status):
        if not os.path.exists(dir_job):
            return
        
        f = open(os.path.join(dir_job, "status"), "w")
        f.write(status)
        f.close()
    
    def __check_job_lock_expired(self, file_lock):
        return (time.time() - os.path.getctime(file_lock) > 60 * 60 * 2)
    
    def __check_download_lock_expired(self, file_lock):
        return (time.time() - os.path.getctime(file_lock) > 60 * 60 * 24 * 7)
    
    def __check_jobs(self):
        for file in os.listdir(self.__dir_jobs):
            dir_job = os.path.join(self.__dir_jobs, file)
            if not os.path.isdir(dir_job):
                continue

            file_job = os.path.join(dir_job, "job")
            if os.path.exists(file_job + ".lock"):
                if self.__check_job_lock_expired(file_job + ".lock"):
                    print "---------------------"
                    print "Job lock expired (" + file_job + ".lock)"
                    self.__delete_job(file_job)
                continue
            
            if not os.path.exists(file_job):
                file_download = os.path.join(dir_job, "download.lock")
                if os.path.exists(file_download):
                    if self.__check_download_lock_expired(file_download):
                        print "---------------------"
                        print "Download lock expired (" + file_download + ")"
                        self.__delete_job(file_job)
                continue

            print "---------------------"
            print "Job found (" + file_job + ")"
            job = self.__read_job(file_job)
            if job != None:
                return job
            
            self.__delete_job(file_job)
            
        return None
    
    def __execute_job(self, job):
        if job.command == "generate":
            print "Command \"generate\" found. Generating map file."
            if job.output_file == None:
                print "No output file set. Aborting."
                self.__delete_job(job.file_job)
                return

            if job.waypoint_file == None and job.bounds == None:
                print "No waypoint file or bounds set. Aborting."
                self.__delete_job(job.file_job)
                return

            if isinstance(job.waypoint_file, basestring):
                job.waypoint_file = job.waypoint_file.replace("\\", "/")
            if isinstance(job.waypoint_details_file, basestring):
                job.waypoint_details_file = job.waypoint_details_file.replace("\\", "/")
            if isinstance(job.airspace_file, basestring):
                job.airspace_file = job.airspace_file.replace("\\", "/")
            if isinstance(job.output_file, basestring):
                job.output_file = job.output_file.replace("\\", "/")
                
            m = MapGenerator()
            if job.waypoint_file != None:
                self.__update_job_status(os.path.dirname(job.file_job), 
                                         "Adding waypoint file...")
                m.AddWaypointFile(os.path.normpath(job.waypoint_file))
                m.SetBoundsByWaypointFile(os.path.normpath(job.waypoint_file))
            if job.bounds != None:
                m.SetBounds(job.bounds)
            
            if job.no_topology != True:
                self.__update_job_status(os.path.dirname(job.file_job), 
                                         "Creating topology files...")
                m.AddTopology()

            if job.no_terrain != True:
                self.__update_job_status(os.path.dirname(job.file_job), 
                                         "Creating terrain files...")
                m.AddTerrain(job.resolution)
            
            if job.waypoint_details_file != None:
                self.__update_job_status(os.path.dirname(job.file_job), 
                                         "Adding waypoint details file...")
                m.AddWaypointDetailsFile(os.path.normpath(job.waypoint_details_file))
                
            if job.airspace_file != None:
                self.__update_job_status(os.path.dirname(job.file_job), 
                                         "Adding airspace file...")
                m.AddAirspaceFile(os.path.normpath(job.airspace_file))
                
            self.__update_job_status(os.path.dirname(job.file_job), 
                                     "Creating map file...")
            m.Create(os.path.normpath(job.output_file))
            m.Cleanup()
            self.__update_job_status(os.path.dirname(job.file_job), 
                                     "Done")
            print "Map ready for use (" + job.output_file + ")"
            self.__lock_download(os.path.dirname(job.file_job))
            self.__delete_job(job.file_job, False)
            return
            
        if job.command == "stop":
            print "Command \"stop\" found. Stopping daemon."
            self.__delete_job(job.file_job)
            self.__run = False
            return
        
        print "No known command given inside MapJob object"
        self.__delete_job(job.file_job)
                
    def Run(self):
        print "Starting MapDaemon ..."
        self.__run = True
        
        print "Monitoring " + self.__dir_jobs + " for new jobs ..."
        while self.__run:
            job = self.__check_jobs()
            if job != None:
                self.__execute_job(job)
            else:
                time.sleep(0.5)
    
if __name__ == '__main__':    
    md = MapDaemon()
    md.Run()
    