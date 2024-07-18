
import numpy as np
import matplotlib.pyplot as plt
# Function to extract RX_BUFF_VAL from a line
def extract_rx_buff_val(line):
    start_index = line.find("RXBUFF_VAL=[") + len("RXBUFF_VAL=[")
    end_index = line.find("];", start_index)
    rx_buff_val = line[start_index:end_index].strip().split()
    return rx_buff_val

# Read the text file and extract RX_BUFF_VAL from each line
def read_text_file(file_path):
    rx_buff_vals = []
    with open(file_path, 'r') as file:
        for line in file:
            if "RXBUFF_VAL=[" in line:
                rx_buff_val = extract_rx_buff_val(line)
                rx_buff_vals.append(rx_buff_val)
    print(type(rx_buff_vals))
    print(len(rx_buff_vals))
    print(len(rx_buff_vals[0]))
    return rx_buff_vals

# Main function
def main():
    # be sure to change to your collected filepath
    file_path = "/Users/zhecun/Desktop/POWDER/RENEWLab/CC/Sounder/output_honorsUSRP.txt"
    rx_buff_vals = read_text_file(file_path)
    # zero-padding (any) jagged list
    maxlen = max(list(map(len, rx_buff_vals)))
    for i in range(len(rx_buff_vals)):
        if len(rx_buff_vals[i]) < maxlen:
            num_zeros = maxlen - len(rx_buff_vals[i])
            rx_buff_vals[i].extend(['0+1j*0'] * num_zeros)

    rx_buff_vals=np.array(rx_buff_vals)

    print("size of rx_buff_vals: {x}".format(x=rx_buff_vals.shape))
    for i in range(rx_buff_vals.shape[0]):
        t = np.arange(maxlen)  # Time axis
        rx_val = rx_buff_vals[i]  # Select sub-array
        rx_val_complex = [elem.replace('1j*', 'i*').replace('i', 'j').replace('+j*', '.').replace('.','+') for elem in rx_val]
        rx_val_complex = [ele + 'j' for ele in rx_val_complex]
        rx_val_complex = [complex(elem.replace('+-', '-')) for elem in rx_val_complex]
        rx_val_complex = np.array(rx_val_complex)
        plt.plot(t, rx_val_complex.real, label=f'Sub-array {i+1}', alpha=.4)  # Plot sub-array with a label
    
    plt.grid(True)
    plt.legend()
    plt.title("BS Iris RF137, RX Ustar, TXgain 100, part2")
    plt.show()

    # print(type(rx_val_complex))
# Entry point of the script
if __name__ == "__main__":
    main()