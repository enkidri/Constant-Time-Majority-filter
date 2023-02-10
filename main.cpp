#include <vector>
#include <algorithm>
#include <thread>

const int HEIGHT = 512;
const int WIDTH = 512;
const int IMG_DEPTH = 256;

int get_index(int x, int y, int ch)
{
    int index = HEIGHT*WIDTH*ch + y*WIDTH + x;
    return index;
}

void operator+=(std::vector<int>& lhs, const std::vector<int>& rhs)
{
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(),
                   std::plus<int>());
}
void operator-=(std::vector<int>& lhs, const std::vector<int>& rhs)
{
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(),
                   std::minus<int>());
}

unsigned char get_majority(std::vector<int>& v)
{
    return std::distance(v.begin(), std::max_element(v.begin(), v.end()));
}

int get_pixel(unsigned char* input_image, int x, int y, int ch)
{
    if ((x < 0) || (y < 0) || (x >= WIDTH) || (y >= HEIGHT))
        return 0;

    int idx = get_index(x, y, ch);
    return *(input_image + idx);
}

class HistogramManager
{
    private:
        struct MultiLevelHist
        {
            std::vector<int> fine;
            std::vector<int> coarse;
        };
        std::vector<int> H;     //kernel histogram
        std::vector<std::vector<int>> h; //Column histograms

        int ksize{};
        unsigned char* input;
        int ch{};

    public:
        HistogramManager(unsigned char* src, int ksize) : input(src), ksize(ksize)
        {
            H.resize(IMG_DEPTH, 0);
            h.resize(WIDTH + 2*ksize, std::vector<int>(IMG_DEPTH,0)); //+2*ksize due to padding on sides
            for (int k=0; k<ksize; k++)
            {
                h[k][0] = 2*ksize+1;
                h[h.size()-1-k][0] = 2*ksize+1;
            }
        }
        void set_ch(int depth)
        {
            ch = depth;
        }

        void clear_kernel()
        {
            std::fill(H.begin(), H.end(), 0);
        }

        void init_col_hist() 
        {
            for (int j=0; j < WIDTH; j++)
            {
                for (int i=-ksize-1; i < ksize; i++)             
                {                                               
                    h[j+ksize][get_pixel(input, j, i, ch)]++;
                }
            }
        }


        void dstep_col_hist(int row)
        // Moves col histograms to current row
        {
            for (int j=0; j < WIDTH; j++)
            {
                int i_add = row + ksize;
                int i_sub = row - ksize - 1;
                h[j+ksize][get_pixel(input, j, i_add, ch)]++;  
                h[j+ksize][get_pixel(input, j, i_sub, ch)]--;
                
            }
        }

        void init_kernel_hist()
        {
            int window_size = 2 * ksize + 1;
            for (int j=0; j < window_size - 1; j++)
            {
                H += h[j];
            }
        }

        void update_pixel(int i, unsigned char* output_image)
        {
            for (int j=0; j < WIDTH; j++)
            {
                int idx_add = j + 2*ksize;      
                int idx_sub = j; 
                H += h[idx_add];

                int idx = get_index(j, i, ch); 
                output_image[idx] = get_majority(H);

                H -= h[idx_sub];
            }
        }

};

void mode_filter(unsigned char* input_image, unsigned char* output_image, int ksize, int ch)
{
    // input_image: 1D unsigned char array containing the greyscale pixel values.
    // output_image: the filtered output of the same size
    // ksize: filter radius
    // ch: the RGB color channel to filter      
            
    HistogramManager hist(input_image, ksize);
    
    hist.set_ch(ch);
    hist.init_col_hist();
    for (int i=0; i < HEIGHT; i++)
    {
        hist.clear_kernel();                //Clears the kernel histogram
        hist.dstep_col_hist(i);             //Moves all column histograms down one step
        hist.init_kernel_hist();
        hist.update_pixel(i, output_image);
    }
}

