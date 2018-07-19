
from os.path import basename
from os.path import splitext

class CodeWriter:

    #  dictionary between section's names to section's convention
    segmentsNameToConventions = {"this": "THIS", "that": "THAT", "local": "LCL", "argument": "ARG",
                                "temp": "5", "pointer": "3"}

    START_LINE = 0

    """
    prepare the output file for writing into it.
    """
    def __init__(self, file):
        self.counter = self.START_LINE
        self.file = open(file, 'w')

    """
    Closing the output file.
    """
    def close(self):
        self.file.close()

    """
    updating the status - tells the writer that the translation of the
    specific file is being started.
    """
    def startTranslatingMessage(self, fileName):
        print ("starting translation file process, file: " + fileName
                + " is going to be translated from VM-code to HACK-assembly")

    def singleInputOp(self, operation):
        stringCommand = "@SP \n" + \
                     "M = M-1 \n" + \
                     "A = M\n" + \
                     "M = " + operation + "M\n"

        stringCommand += "@SP\n" + "M = M+1\n"
        return stringCommand

    """
    perform operations involves 2 inputs from the stack
    """
    def doubleInputOp(self, operation):
        stringCommand = "@SP\n" + "M = M-1\n" + "A = M\n" + "D = M \n"
        stringCommand += "@SP\n" +"M = M-1\n" + "A = M\n"
        stringCommand += "M = M " + operation + " D \n"  # perform the specific operation
        stringCommand += "@SP\n" + "M = M+1\n"
        return stringCommand


    """
    perform a comparision operation such as: >, <, =
    """
    def compareOperation(self, command):
        self.counter += 1

        COMMAND_UPPERCASE = command.upper()
        CURRENT_LINE = str(self.counter)

        # D,R15 gets Y's value
        commandString = "@SP\n" + \
                     "M = M-1\n" + \
                     "A = M\n" + \
                     "D = M\n" + \
                     "@R15\n" + \
                     "M = D\n"

        # jump if Y is negative
        commandString += "@yIsNegative" + CURRENT_LINE + " \n" + \
                     "D;JLT\n" + \
                     "@SP\n" + \
                     "M = M-1\n" + \
                     "A = M\n" + \
                     "D = M\n"

        # jump if Y is positive & X is negative
        commandString += "@yIsPositiveXIsNegative" + CURRENT_LINE + " \n" + \
                     "D;JLT\n"

        # Y and X are positive values. so D=Y-X
        commandString += "@R15\n" + \
                     "D = D-M\n" + \
                     "@TESTComparision" + CURRENT_LINE + "\n" + \
                     "0; JMP\n"

        # Y is neg so D gets X's value
        commandString += "(yIsNegative" + CURRENT_LINE + ") \n" + \
                     "@SP\n" + \
                     "M = M - 1\n" + \
                     "A = M\n" + \
                     "D = M \n"

        commandString += "@yIsNegativeXIsPositive" + CURRENT_LINE + " \n" + \
                     "D;JGT\n"

        #  X is neg and Y is neg so D=X-Y
        commandString += "@R15\n" + \
                     "D = D-M\n" + \
                     "@TESTComparision" + CURRENT_LINE + "\n" + \
                     "0; JMP\n"

        commandString += "(yIsPositiveXIsNegative" + CURRENT_LINE + ")\n" + \
                     "D = -1\n" + \
                     "@TESTComparision" + CURRENT_LINE + "\n" + \
                     "0; JMP\n"

        commandString += "(yIsNegativeXIsPositive" + CURRENT_LINE + ") \n" + \
                     "D = 1\n" + \
                     "@TESTComparision" + CURRENT_LINE + "\n" + \
                     "0;JMP\n"

        commandString += "(TESTComparision" + CURRENT_LINE + ") \n" + \
                     "@TRUE" + CURRENT_LINE + "\n" + \
                     "D;" + ("J" + COMMAND_UPPERCASE) + " \n" + \
                     "D = 0\n" + "@sp\n"

        commandString += "@END" + CURRENT_LINE + " \n" + \
                     "0;JMP \n"

        commandString += "(TRUE" + CURRENT_LINE + ")\n" + \
                     "D = -1\n" + \
                     "@END" + CURRENT_LINE + "\n" + \
                     "0;JMP\n"

        commandString += "(END" + CURRENT_LINE + ")\n" + \
                     "@SP\n" + \
                     "A=M\n" + \
                     "M=D\n"

        # let the result be the correct result D=-1 stands for flase so it becomes 0,
        # D=0 stands for true so it becomes 1.
        commandString += "@SP\n" + \
                     "M = M + 1\n"

        return commandString

    """
    Writes the assembly code that is the translation of the given arithmetic command.
    """
    def arithmeticWriter(self, command):

        #  single input operation
        if command == "not":
            commandString = self.singleInputOp("!")
        elif command == "neg":
            commandString = self.singleInputOp("-")

        #  double input operation
        elif command == "sub":
            commandString = self.doubleInputOp("-")
        elif command == "add":
            commandString = self.doubleInputOp("+")
        elif command == "or":
            commandString = self.doubleInputOp("|")
        elif command == "and":
            commandString = self.doubleInputOp("&")

        else: # so it must be a compare operation such as <,>,=
            commandString = self.compareOperation(command)

        self.file.write(commandString)

    """
    performing the push operation to the stack. (push D to stack)
    """
    def pushToStack(self):
        commandString = "@SP\n"+"A = M\n"+"M = D\n"+"@SP\n"+ "M = M+1\n"
        return commandString

    def pop(self, segment, index):
        commandStr = "@" + index + "\n" + "D  = A\n" + \
                     "@"+self.segmentsNameToConventions[segment] + "\n"

        thisOrThat = (segment == "that") or (segment == "this")
        regularSegment = thisOrThat or (segment == "local") or (segment == "argument")

        if regularSegment:
            commandStr += "A = M\n"

        # perform the pop operation
        commandStr += "D = A + D\n" + "@R15\n" + "M = D\n" # R15 is the argument's index we should store in
        commandStr += "@SP\n" + "M = M-1\n" + "A = M\n"
        commandStr += "D=M\n" + "@R15\n" + "A = M\n" + "M = D\n"

        return commandStr


    def getFileName(self):
        FILE_NAME_POSITION = 0

        fileName =  basename(self.file.name)
        fileName = splitext(fileName)[FILE_NAME_POSITION]
        return fileName


    """
    taking care of static pop
    """
    def staticPop(self, fileName, segmentIndex):
        stringCommand = "@SP\n"+"M = M-1\n"
        stringCommand += "A = M\n" + "D = M\n"
        stringCommand += "@" + fileName + "." + segmentIndex + "\n" + "M = D\n"  # store the static value
        return stringCommand


    def pushPopWriter(self, command, segment, segmentIndex):
        """
        writing the assembly code of push or pop operations to the output file.
        VAR command - is a string represents one of the operation pop\push.
         gets one of the values "C_PUSH" or "C_POP".
        VAR segment - string represents the segment, of the pop\push operation
        VAR segmentIndex - the index of the specific segment we deal with
        """
        stringCommand = ''

        fileName = self.getFileName()

        tempOrPointerCommand = segment == "temp" or segment == "pointer"
        thisThatLocalArgCommand = segment == "this" or segment == "that"\
                                  or segment == "local" or segment == "argument"

        if command == "C_PUSH":
            if tempOrPointerCommand:
                stringCommand = "@" + segmentIndex + "\n" + "D = A\n" + \
                                "@" + self.segmentsNameToConventions[segment] + "\n" + \
                                "A = A + D\n" + \
                                "D = M\n" + \
                                self.pushToStack()

            elif thisThatLocalArgCommand:
                stringCommand = "@" + segmentIndex + "\n" + "D = A\n" + \
                             "@"+self.segmentsNameToConventions[segment] + "\n" + \
                             "A = M+D\n" + "D = M \n" + self.pushToStack()

            elif segment == "constant":  # push constant-value !
                stringCommand = "@" + segmentIndex + "\n" + "D = A\n" + self.pushToStack()

            elif segment == "static":
                stringCommand = "@"+fileName + "." + segmentIndex +" \n" + "D = M\n" + self.pushToStack()

        else: # pop (regular or static)
            if segment == "static":
                stringCommand = self.staticPop(fileName, segmentIndex)
            else:
                stringCommand = self.pop(segment, segmentIndex)

        self.file.write(stringCommand)

