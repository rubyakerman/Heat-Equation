
"""
parse the input file & getting its commands.
"""
class Parser:

    ARITHMETIC = "C_ARITHMETIC"
    POP = "C_POP"
    PUSH = "C_PUSH"


    """
    constructor - get input file path, opens it & clean comments, empty lines & '\n'.
    """
    def __init__(self, file):
        NO_COMMAND = ""
        FIRST_LINE = 0

        self.file = open(file)
        self.lineCounter = FIRST_LINE
        self.currentCommand = NO_COMMAND
        self.lines = self.file.readlines()   # splitting lines

    def getCurrentCommand(self):
        return self.currentCommand

    """
    spliting lines, removing '\n', comments & empty lines from file 
    """
    def cleaningFile(self):
        COMMENTS = "//"
        EMPTY = ''
        NON_COMMENT_POSITION = 0

        self.lines = [line.replace('\n', EMPTY) for line in self.lines]  # removing '\n' from file
        self.lines = [line.replace('\r', EMPTY) for line in self.lines]  # removing '\r' from file
        self.lines = [line.split(COMMENTS)[NON_COMMENT_POSITION].strip() for line in self.lines] # removes comments
        self.lines = [line for line in self.lines if line != EMPTY] # removes empty lines

    """
    checking weather the input file has more commands to perform (commands we have'nt read yet). 
    """
    def hasMoreCommands(self):
        return ( self.lineCounter < len(self.lines) )

    """
    advance the line by one by reading the next command from the input file,
    and return the advanced line
    Defines it to be the current command.
    advance() should be called only if hasMoreCommands () is true.
    at the beginning there is'nt current-command.
    """
    def advance (self):
        # lineCounter always pointing to the next command line should be excecuted:
        self.currentCommand = self.lines[self.lineCounter]  # read next command
        self.lineCounter += 1 # advance for pointing the next command should be read

        return self.currentCommand

    """
    return the type of the current command
    """
    def commandType(self):
        if "pop" in self.currentCommand:
            return self.POP

        if "push" in self.currentCommand:
            return self.PUSH

        if ( "and" or "or" or "not" or "add" or "sub" or "neg"
                            or "eq" or "lt" or "gt" ) in self.currentCommand:

            return self.ARITHMETIC

    """
    return the current command's first argument
    """
    def getArg1(self):
        FIRST_ARG_POSITION = 1

        if (self.commandType() is not self.POP) and (self.commandType() is not self.PUSH):
            return self.currentCommand

        else: # this is pop or push command so:
            return self.currentCommand.split() [FIRST_ARG_POSITION]

    """
    Return the current command's second argument
    """
    def getArg2(self):
        SECOND_ARG_POSITION = 2

        return self.currentCommand.split() [SECOND_ARG_POSITION]