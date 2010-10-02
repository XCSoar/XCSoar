import os
import time

from map_generator import MapGenerator
import pickle

class MapJob:
    use_waypoint_file = False
    use_waypoint_details_file = False
    use_airspace_file = False
    use_topology = True
    use_terrain = True
    bounds = None
    resolution = 9.0
    
    def __init__(self, command = "generate"):
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
        job.dir_job = os.path.dirname(file_job)
        
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
            if job.use_waypoint_file == False and job.bounds == None:
                print "No waypoint file or bounds set. Aborting."
                self.__delete_job(job.file_job)
                return

            m = MapGenerator()
            if job.use_waypoint_file:
                self.__update_job_status(job.dir_job, 
                                         "Adding waypoint file...")
                m.AddWaypointFile(os.path.join(job.dir_job, "waypoints.dat"))
                m.SetBoundsByWaypointFile(os.path.join(job.dir_job, "waypoints.dat"))
            if job.bounds != None:
                m.SetBounds(job.bounds)
            
            if job.use_topology:
                self.__update_job_status(job.dir_job, 
                                         "Creating topology files...")
                m.AddTopology()

            if job.use_terrain:
                self.__update_job_status(job.dir_job, 
                                         "Creating terrain files...")
                m.AddTerrain(job.resolution)
            
            if job.use_waypoint_details_file:
                self.__update_job_status(job.dir_job, 
                                         "Adding waypoint details file...")
                m.AddWaypointDetailsFile(os.path.join(job.dir_job, "details.txt"))
                
            if job.use_airspace_file:
                self.__update_job_status(job.dir_job, 
                                         "Adding airspace file...")
                m.AddAirspaceFile(os.path.join(job.dir_job, "sua.txt"))
                
            self.__update_job_status(job.dir_job, 
                                     "Creating map file...")
            m.Create(os.path.join(job.dir_job, "map.xcm"))
            m.Cleanup()
            self.__update_job_status(job.dir_job,  
                                     "Done")
            print "Map ready for use (" + os.path.join(job.dir_job, "map.xcm") + ")"
            self.__lock_download(job.dir_job)
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
    