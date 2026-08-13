/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /*
   * MPU FAULT CAUGHT!
   * A rogue pointer or task attempted an illegal memory access.
   * In a student project, this would silently freeze in the infinite while(1) loop below.
   * In our aerospace framework, we trigger an emergency reboot to clear the memory corruption.
   */

  NVIC_SystemReset();

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}
