#include <cppon/c++on.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

// Utility function to print a blob's content in hexadecimal
void print_hex(const ch5::blob_t& blob, size_t max_bytes = 32) {
    std::cout << "Data (hex): ";
    for (size_t i = 0; i < std::min(blob.size(), max_bytes); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(blob[i]) << " ";
    }
    if (blob.size() > max_bytes) {
        std::cout << "... (" << std::dec << blob.size() << " total bytes)";
    }
    std::cout << std::dec << std::endl;
}

int main() {
    std::cout << "=== Binary blobs in C++ON ===\n\n";

    // Parse JSON that contains a blob
    std::cout << "Parsing JSON with a blob:\n";
    auto object = ch5::eval("{\"blob\":\"$cppon-blob:SGVsbG8sIFdvcmxkIQ==\"}", ch5::Full);

    // Extract the blob
    auto blob = ch5::get_blob(object["/blob"]);

    // Show blob information
    std::cout << "Blob size: " << blob.size() << " bytes\n";
    print_hex(blob);

    // Convert to string to display content
    std::string text_content(blob.begin(), blob.end());
    std::cout << "Text content: \"" << text_content << "\"\n\n";

    // Manual blob creation
    std::cout << "=== Creating a blob and storing it in a document ===\n";

    // Create a blob from a byte array
    ch5::blob_t manual_blob = {
        0x42, 0x69, 0x6E, 0x61, 0x72, 0x79,  // "Binary"
        0x20, 0x44, 0x61, 0x74, 0x61         // " Data"
    };

    // Store in a document
    ch5::cppon doc;
    doc["/binary_data"] = manual_blob;

    // Add metadata
    doc["/metadata/type"] = "text";
    doc["/metadata/encoding"] = "ASCII";
    doc["/metadata/size"] = manual_blob.size();

    // Print the document with the blob
    std::cout << "Document with blob:\n"
        << ch5::to_string(doc, "{\"compact\":false}") << "\n\n";

    // Extraction and manipulation
    std::cout << "=== Blob extraction and manipulation ===\n";

    // Get blob by reference (decodes if needed)
    auto& mutable_blob = ch5::get_blob(doc["/binary_data"]);

    // Modify blob
    mutable_blob.push_back(0x21);  // '!'

    std::cout << "Blob after modification:\n";
    print_hex(mutable_blob);

    // Convert to string for display
    std::string modified_text(mutable_blob.begin(), mutable_blob.end());
    std::cout << "Modified content: \"" << modified_text << "\"\n";

    // Example: store a small image
    std::cout << "\n=== Example: image storage ===\n";

    ch5::cppon image;
    image["/width"] = 16;
    image["/height"] = 16;
    image["/format"] = "RGB";

    // Simulated image data (16x16 red pixels)
    ch5::blob_t image_data;
    for (int i = 0; i < 16 * 16; ++i) {
        image_data.push_back(0xFF);  // R
        image_data.push_back(0x00);  // G
        image_data.push_back(0x00);  // B
    }

    image["/data"] = image_data;

    std::cout << "Image stored with " << image_data.size()
        << " bytes of binary data\n";

    // Show JSON (truncate data for readability)
    image["/data"] = "$cppon-blob:<768 bytes of binary data>";
    std::cout << ch5::to_string(image, "{\"compact\":false}") << "\n";

    return 0;
}