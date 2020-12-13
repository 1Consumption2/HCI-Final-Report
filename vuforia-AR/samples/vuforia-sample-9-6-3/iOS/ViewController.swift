/*===============================================================================
Copyright (c) 2020, PTC Inc. All rights reserved.
 
Vuforia is a trademark of PTC Inc., registered in the United States and other
countries.
===============================================================================*/
import UIKit

class ViewController: UIViewController, UICollectionViewDelegate {
    
    struct Constants {
        static let IMAGE_TARGET_SEGUE_ID = "ImageTargetSegue"
        static let MODEL_TARGET_SEGUE_ID = "ModelTargetSegue"
    }

    @IBOutlet weak var imageTargetImageView: UIImageView!
    @IBOutlet weak var modelTargetImageView: UIImageView!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        let imageTapRecognizer = UITapGestureRecognizer(target: self, action: #selector(self.navigateToImageTargets))
        imageTapRecognizer.numberOfTapsRequired = 1
        imageTargetImageView.addGestureRecognizer(imageTapRecognizer)
        
        let modelTapRecognizer = UITapGestureRecognizer(target: self, action: #selector(self.navigateToModelTargets))
        modelTapRecognizer.numberOfTapsRequired = 1
        modelTargetImageView.addGestureRecognizer(modelTapRecognizer)
    }

    
    @objc func navigateToImageTargets() {
        performSegue(withIdentifier: Constants.IMAGE_TARGET_SEGUE_ID, sender: nil)
    }
    
    
    @objc func navigateToModelTargets() {
        performSegue(withIdentifier: Constants.MODEL_TARGET_SEGUE_ID, sender: nil)
    }

    
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        if let vuforiaViewController = segue.destination as? VuforiaViewController {
            if (segue.identifier == Constants.IMAGE_TARGET_SEGUE_ID) {
                vuforiaViewController.mTarget = getImageTargetId()
            } else if (segue.identifier == Constants.MODEL_TARGET_SEGUE_ID) {
                vuforiaViewController.mTarget = getModelTargetId()
            }
        }
    }
}
