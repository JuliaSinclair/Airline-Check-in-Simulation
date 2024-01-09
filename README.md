# Airline-Check-in-Simulation
Assignment for Operating Systems (Fall 2022) at University of Victoria

To compile and run the code: <br>
    From UVic's Linux Environment, compile code by executing "make" from terminal while in the directory containing assignment files. 
    To run code, execute "./ACS <inputfilename>.txt" from terminal. The input file should be in the following format and contain appropriate
    values. Inputting files with improper formatting or incorret values will results in an error message and the program exiting.

Input File Format:<br>
    The input file is a text file and has a simple format. The first line contains the total number of customers that will
    be simulated. After that, each line contains the information about a single customer, such that:<br>
    1. The first character specifies the unique ID of customers.<br>
    2. A colon(:) immediately follows the unique number of the customer.<br>
    3. Immediately following is an integer equal to either 1 (indicating the customer belongs to business class) or 0
    (indicating the customer belongs to economy class).<br>
    4. A comma(,) immediately follows the previous number.<br>
    5. Immediately following is an integer that indicates the arrival time of the customer.<br>
    6. A comma(,) immediately follows the previous number.<br>
    7. Immediately following is an integer that indicates the service time of the customer.<br>
    8. A newline (\n) ends a line.<br>
    NOTE: All times are measured in 10ths of a second.

Program Output: <br>
    For each customer specified in input file, the following information will be outputted. <br>
    The order of the printed information is variable and dependent on the input file values.<br>
    1. A customer arrives: customer. ID<br>
    2. A customer enters a queue: the queue ID, and length of the queue.<br>
    3. A clerk starts serving a customer: start time, the customer ID, the clerk ID.<br>
    4. A clerk finishes serving a customer: end time, the customer ID, the clerk ID.<br>
    NOTE: All times are in relative machine time.

At the end of the program, the following information will be outputted. <br>
    1. Average waiting time of all customers in the system<br>
    2. Average waiting time of all business-class customers<br>
    3. Average waitingtime of all economy-class customers<br>
    NOTE: Waiting time of a customer is defined as the relative machine time from when the
    customer enters a queue to when a clerk starts servicing the customer.

