class FileList:
    def __init__(self):
        self.__list = []

    def __iter__(self):
        return iter(self.__list)

    def clear(self):
        self.__list = []

    def extend(self, list):
        if not isinstance(list, FileList):
            raise TypeError
        self.__list.extend(list)

    def add(self, file, compress):
        self.__list.append((file, compress))
